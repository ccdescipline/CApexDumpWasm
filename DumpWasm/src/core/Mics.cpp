#include "Mics.h"
#include "DumpUtils.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace {

uint64_t FindLastVisibleTimeOffset(const dumpContext& ctx) {
    // lastVisibleTime 没有稳定的长sig，改走字符串xref定位：先找字符串，再找引用它的代码。
    auto findStringOffsets = [&](const char* needle) {
        auto matches = std::vector<uint64_t>();
        auto mask = std::string(std::strlen(needle), 'x');

        auto rdataMatches = PS::SearchInSectionMultiple(ctx.data.data(), ".rdata", needle, mask.c_str());
        matches.insert(matches.end(), rdataMatches.begin(), rdataMatches.end());

        auto dataMatches = PS::SearchInSectionMultiple(ctx.data.data(), ".data", needle, mask.c_str());
        matches.insert(matches.end(), dataMatches.begin(), dataMatches.end());

        if (matches.empty()) {
            // 正常应在.rdata/.data，fallback全量搜索是为了兼容section名或布局变化。
            auto fullMatches = PS::SearchMultiple(ctx.data.data(), ctx.data.size(), needle, mask.c_str());
            matches.insert(matches.end(), fullMatches.begin(), fullMatches.end());
        }

        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
        return matches;
    };

    auto findLeaXrefsToOffset = [&](uint64_t targetOffset) {
        // x64字符串参数通常是 lea r?, [rip+disp32]，这里反算disp32确认是否指向目标字符串。
        auto xrefs = std::vector<uint64_t>();

        auto appendLeaMatches = [&](const char* sig) {
            auto matches = PS::SearchInSectionMultiple(ctx.data.data(), ".text", sig, "xxx????");
            for (auto leaOffset : matches) {
                if (leaOffset + 7 > ctx.data.size()) continue;

                auto target = static_cast<int64_t>(leaOffset) + 7 + DumpUtils::ReadI32(ctx.data, leaOffset + 3);
                if (target < 0 || static_cast<uint64_t>(target) != targetOffset) continue;

                xrefs.push_back(leaOffset);
            }
        };

        appendLeaMatches("\x48\x8D\x15\x00\x00\x00\x00"); // lea rdx, [rip+disp32]
        appendLeaMatches("\x48\x8D\x0D\x00\x00\x00\x00"); // lea rcx, [rip+disp32]
        appendLeaMatches("\x4C\x8D\x05\x00\x00\x00\x00"); // lea r8, [rip+disp32]
        appendLeaMatches("\x4C\x8D\x0D\x00\x00\x00\x00"); // lea r9, [rip+disp32]

        std::sort(xrefs.begin(), xrefs.end());
        xrefs.erase(std::unique(xrefs.begin(), xrefs.end()), xrefs.end());
        return xrefs;
    };

    auto findEntityFieldStoreAfterXref = [&](uint64_t xrefOffset) -> uint64_t {
        // 字符串xref后面的注册逻辑会把 entity 字段写到返回指针：mov reg32,[rbx+offset] -> mov [rax],reg32。
        const auto* bytes = reinterpret_cast<const uint8_t*>(ctx.data.data());
        const auto end = std::min<uint64_t>(ctx.data.size(), xrefOffset + 0x100);

        for (auto offset = xrefOffset; offset + 6 <= end; ++offset) {
            if (bytes[offset] != 0x8B) continue;

            auto modrm = bytes[offset + 1];
            // mod=10b 表示disp32，r/m=011b 表示rbx：即 mov reg32, [rbx+disp32]。
            if ((modrm & 0xC0) != 0x80 || (modrm & 0x07) != 0x03) continue;

            auto reg = (modrm >> 3) & 7;
            auto fieldOffset = DumpUtils::ReadU32(ctx.data, offset + 2);
            if (!fieldOffset || fieldOffset > 0x4000) continue;

            const auto storeEnd = std::min<uint64_t>(end, offset + 6 + 0x10);
            for (auto storeOffset = offset + 6; storeOffset + 2 <= storeEnd; ++storeOffset) {
                if (bytes[storeOffset] != 0x89) continue;

                auto storeModrm = bytes[storeOffset + 1];
                // 目标必须是[rax]，源寄存器必须和上一条load一致，避免误取附近无关字段。
                if ((storeModrm & 0xC0) != 0x00 || (storeModrm & 0x07) != 0x00) continue;
                if (((storeModrm >> 3) & 7) != reg) continue;

                return fieldOffset;
            }
        }

        return 0;
    };

    auto stringOffsets = findStringOffsets("lastVisibleTime");
    for (auto stringOffset : stringOffsets) {
        auto xrefs = findLeaXrefsToOffset(stringOffset);
        for (auto xref : xrefs) {
            auto fieldOffset = findEntityFieldStoreAfterXref(xref);
            if (fieldOffset) {
                LogE("lastVisibleTime xref 0x%llx field 0x%llx", xref, fieldOffset);
                return fieldOffset;
            }
        }
    }

    return 0;
}

} // namespace

bool Mics::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errors) {
    // [SIG] LocalPlayer - 通过virtual call加载LocalPlayer全局指针
    // 反汇编: lea rcx, [rip+LocalPlayer]  ; 48 8D 0D xx xx xx xx
    //         mov rdx, rdi                ; 48 8B D7
    //         call qword ptr [rax+0x58]   ; FF 50 58
    // 如何找到: [无直接字符串引用] 该函数通过vtable间接调用进入, 包含大量fmaxf/fminf角度clamping
    // 注意: 代码里对结果 += 8 (LocalPlayer变量实际存在这个地址上)
    uint64_t LocalPlayer = Pattern::FindPattern<uint64_t>(ctx.data, ("48 8D 0D ? ? ? ? 48 8B D7 FF 50 58"), 7);
    LogE("LocalPlayer : 0x%llx", LocalPlayer);
    if (!LocalPlayer) {
        errors.push_back("LocalPlayer not found");
    } else {
        LocalPlayer += 8;
        output["LocalPlayer"] = LocalPlayer;
    }

    // [SIG] EntityList - call之后紧跟的lea加载EntityList
    // 反汇编: call sub_XXX              ; E8 xx xx xx xx
    //         lea rbx, [rip+EntityList] ; 48 8D 1D xx xx xx xx
    //         cmp eax, 1                ; 83 F8 01
    // 如何找到: [无直接字符串引用] 函数通过vtable间接调用进入, 无法通过字符串定位
    // RVA=12: 解析第2条指令 lea rbx (从匹配起始+5开始7字节)
    uint64_t EntityList = (uint64_t)Pattern::FindPattern(ctx.data, ("E8 ? ? ? ? 48 8D 1D ? ? ? ? 83 F8 01"), 12);
    LogE("EntityList : 0x%llx", EntityList);
    if (!EntityList) {
        errors.push_back("EntityList not found");
    } else {
        output["EntityList"] = EntityList;
    }

    // [SIG] ViewRender - 通过vtable偏移0x70调用的渲染对象
    // 反汇编: mov rcx, [rip+ViewRender]  ; 48 8B 0D xx xx xx xx
    //         mov edx, imm32             ; BA xx xx xx xx
    //         mov rax, [rcx]             ; 48 8B 01
    //         call qword ptr [rax+0x70]  ; FF 50 70
    //         lea rdx, [rip+?]           ; 48 8D 15 ...
    // 如何找到: [无直接字符串引用] 位于IDA未识别的代码区域, 只能靠字节搜索
    uint64_t ViewRender = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 01 FF 50 70 48 8D 15"), 7);
    LogE("ViewRender : 0x%llx", ViewRender);
    if (!ViewRender) {
        errors.push_back("ViewRender not found");
    } else {
        output["ViewRender"] = ViewRender;
    }

    // [SIG] ViewMatrix - 写入矩阵后存全局指针
    // 反汇编: movups [rdi+ViewMatrix], xmm3  ; 0F 11 9F xx xx xx xx
    //         mov [rip+?], rdi              ; 48 89 3D xx xx xx xx
    // proc返回 *(UINT32*)(addr+17) = 第二条指令 mov [rip+X] 的disp32
    // 如何找到: [无直接字符串引用] 位于IDA未识别的代码区域, 前后大量movups/movaps矩阵操作
    uint64_t ViewMatrix = (uint64_t)Pattern::FindPatternByProc(ctx.data, ("0F 11 9F ? ? ? ? 48 89 3D ? ? ? ?"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 17));
    });
    LogE("ViewMatrix : 0x%llx", ViewMatrix);
    if (!ViewMatrix) {
        errors.push_back("ViewMatrix not found");
    } else {
        output["ViewMatrix"] = ViewMatrix;
    }

    // [SIG] NameList - 通过entity offset 0x38索引的名字表
    // 反汇编: movsxd rax, [rbx+0x38]      ; 48 63 43 38   (读entity的name index)
    //         lea rcx, [rip+NameList]     ; 48 8D 0D xx xx xx xx
    //         lea rax, [rax+rax*2]        ; 48 8D 04 40   (index*3)
    //         mov rax, [rcx+rax*8-0x18]   ; 48 8B 44 C1 E8
    // 如何找到: [无直接字符串引用] 该pattern有5个匹配, FindPattern取第一个
    //   特征是 48 63 43 38 前缀(entity->nameIndex in rax)
    uint64_t NameList = (uint64_t)Pattern::FindPattern(ctx.data, ("48 63 43 38 48 8D 0D ? ? ? ? 48 8D 04 40 48 8B 44 C1 E8"), 11);
    LogE("NameList : 0x%llx", NameList);
    if (!NameList) {
        errors.push_back("NameList not found");
    } else {
        output["NameList"] = NameList;
    }

    // [SIG] GlobalVars - 作为rdx参数传给虚函数[rax+0x10]
    // 反汇编: lea rdx, [rip+GlobalVars]   ; 48 8D 15 xx xx xx xx
    //         call qword ptr [rax+0x10]   ; FF 50 10
    //         test eax, eax               ; 85 C0
    // 如何找到: [无直接字符串引用] 位于IDA未识别的代码区域, 只能靠字节搜索
    uint64_t GlobalVars = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 15 ? ? ? ? FF 50 10 85 C0"), 7);
    LogE("GlobalVars : 0x%llx", GlobalVars);
    if (!GlobalVars) {
        errors.push_back("GlobalVars not found");
    } else {
        output["GlobalVars"] = GlobalVars;
    }

    // [SIG] NetworkVarTablePtr - 网络变量表查询函数中的lea
    // 反汇编: lea r?, [rip+?]           ; 48 8D ?? xx xx xx xx
    //         cmp dword [r?+?], ?       ; 83 7C ? ? ?
    //         jz +?                     ; 74 ?
    //         mov rdx, rdi              ; 48 8B D7
    //         lea rcx, [rip+NetVarTab]  ; 48 8D 0D xx xx xx xx
    //         call sub_XXX              ; E8 xx xx xx xx
    //         jmp +?                    ; EB
    // 如何找到: 搜索字符串 "Network var \"%s\" is not a time" → xref进入函数
    //   sig就在该函数中间, 紧挨 cmp/jz 分支之前
    // RVA=7: 注意这里解析的是模式起始位置的 lea (第1条指令)
    uint64_t NetworkVarTablePtr = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D ? ? ? ? ? 83 7C ? ? ? 74 ? 48 8B D7 48 8D 0D ? ? ? ? E8 ? ? ? ? EB"), 7);
    LogE("NetworkVarTablePtr : 0x%llx", NetworkVarTablePtr);
    if (!NetworkVarTablePtr) {
        errors.push_back("NetworkVarTablePtr not found");
    } else {
        output["NetworkVarTablePtr"] = NetworkVarTablePtr;
    }

    // [SIG] InputSystem - 输入系统全局指针
    // 反汇编: lea rcx, [rip+InputSystem]  ; 48 8D 0D xx xx xx xx
    //         test ebp, ebp               ; 85 FD
    //         jz +?                       ; 74 ?
    //         cmp ebx, r14d               ; 41 3B DE
    // 如何找到: [无直接字符串引用] 函数通过WndProc消息处理链调用, 整条调用链均无字符串
    uint64_t InputSystem = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 0D ? ? ? ? 85 FD 74 ? 41 3B DE"), 7);
    LogE("InputSystem : 0x%llx", InputSystem);
    if (!InputSystem) {
        errors.push_back("InputSystem not found");
    } else {
        output["InputSystem"] = InputSystem;
    }

    // [SIG] LevelName - 当前关卡名字符串指针
    // 反汇编: lea rdx, [rip+LevelName]  ; 48 8D 15 xx xx xx xx
    //         mov rax, [rbp-8]          ; 48 8B 45 F8
    //         lea rcx, [rip+?]          ; 48 8D 0D xx xx xx xx
    // 如何找到: 搜索字符串 "'%s' %s %u potentially visible objects" → xref进入函数
    //   LevelName 的 lea rdx 就在该字符串引用附近, 作为 %s 参数传入
    uint64_t LevelName = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 15 ? ? ? ? 48 8B 45 F8 48 8D 0D ? ? ? ?"), 7);
    LogE("LevelName : 0x%llx", LevelName);
    if (!LevelName) {
        errors.push_back("LevelName not found");
    } else {
        output["LevelName"] = LevelName;
    }

    // [SIG] highlightSetting - 高亮设置数组基址
    // 反汇编: mov rax, [rip+highlight]   ; 48 8B 05 xx xx xx xx
    //         mov edx, [rcx+rax+0x30]   ; 8B 54 01 30
    // 如何找到: 搜索字符串 "HighlightContext_GetInsideFunction" → xref进入脚本注册函数
    //   该注册函数会调用本函数(sub_7FF72D2F03A0), 在本函数开头附近找到 mov rax,[rip+X]
    //   或: 搜索 "HighlightContext_GetOutlineFunction" / "HighlightContext_GetParam" 等
    //   它们都在同一个脚本回调注册函数里, 该函数调用链可达本sig所在函数
    uint64_t highlightSetting = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8B 05 ? ? ? ? 8B 54 01 30"), 7);
    LogE("highlightSetting : 0x%llx", highlightSetting);
    if (!highlightSetting) {
        errors.push_back("highlightSetting not found");
    } else {
        output["highlightSetting"] = highlightSetting;
    }

    // [SIG] WeaponSettingsMeta_base - weapon实例中的settings meta指针偏移
    // 反汇编: mov r8, [rbx+offset1]    ; 4C 8B 83 xx xx xx xx  (读weapon.setting指针)
    //         lea r9, [rbx+offset2]    ; 4C 8D 8B xx xx xx xx  (计算WeaponSettingsMeta_base地址)
    // proc返回 *(UINT32*)(addr+10) = 第2条指令的disp32 = offset2
    // 如何找到: [无直接字符串引用] 函数被多个武器相关逻辑调用, 调用链均无字符串
    uint64_t WeaponSettingsMeta_base = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("4C 8B 83 ? ? ? ? 4C 8D 8B"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 10));
    });
    LogE("WeaponSettingsMeta_base : 0x%llx", WeaponSettingsMeta_base);
    if (!WeaponSettingsMeta_base) {
        errors.push_back("WeaponSettingsMeta_base not found");
    } else {
        output["WeaponSettingsMeta_base"] = WeaponSettingsMeta_base;
    }

    // [SIG] studioHdr - entity中的studioHdr指针偏移
    // 反汇编: mov rbx, [rdi+studioHdrOff]  ; 48 8B 9F xx xx xx xx
    //         test rbx, rbx                ; 48 85 DB
    //         jnz +0x0C                    ; 75 0C
    // proc返回 *(UINT32*)(addr+3) = mov指令的disp32 = entity中studioHdr字段的偏移
    // 如何找到: 搜索字符串 "Entity has no model" → xref进入函数
    //   函数开头: if(!studioHdr) error("Entity has no model")
    //   studioHdr 就是开头 mov rbx,[rdi+X] 指令中的偏移X
    //   或搜: "Entity has no sequences" / "Parameter index invalid." → 同一个函数
    uint64_t studioHdr = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("48 8B 9F ? ? ? ? 48 85 DB 75 0C"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("studioHdr : 0x%llx", studioHdr);
    if (!studioHdr) {
        errors.push_back("studioHdr not found");
    } else {
        output["studioHdr"] = studioHdr;
    }

    // [XREF] lastVisibleTime - entity可见性时间戳偏移
    // 反汇编: lea rdx, [rip+"lastVisibleTime"] → call注册/查找 → mov ecx, [rbx+offset] → mov [rax], ecx
    // 如何找到: 搜索字符串 "lastVisibleTime" → 找.text里的RIP相对lea xref → xref后取第一个mov reg32,[rbx+offset]
    uint64_t lastVisibleTime = FindLastVisibleTimeOffset(ctx);
    if (!lastVisibleTime) {
        errors.push_back("lastVisibleTime not found");
    } else {
        output["lastVisibleTime"] = lastVisibleTime;
    }

    // [SIG] m_viewangle - player视角角度偏移
    // 反汇编: mov [rbx+viewAngleX], eax / mov [rbx+viewAngleY], eax / mov [rbx+viewAngleZ], eax
    // proc返回 *(UINT32*)(addr+2) = 第一条 mov [rbx+X], eax 的disp32
    uint64_t m_viewangle = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("89 83 ? ? ? ? 8B 47 ? 89 83 ? ? ? ? 8B 47 ? 89 83 ? ? ? ? 48 8B 05 ? ? ? ? 83 78 ? 00"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 2));
    });
    LogE("m_viewangle : 0x%llx", m_viewangle);
    if (!m_viewangle) {
        errors.push_back("m_viewangle not found");
    } else {
        output["m_viewangle"] = m_viewangle;
    }

    // [SIG] m_vecAbsOrigin - entity绝对坐标偏移
    // 反汇编: movq xmm2, qword ptr [rdi+absOriginOff]  ; F3 0F 7E 97 xx xx xx xx
    // 极短pattern(4字节), 但在首个匹配位置提取 addr+4 处的disp32 = entity偏移
    // 如何找到: [无直接字符串引用] 函数通过vtable间接调用进入, 包含大量SIMD运动/位置计算
    uint64_t m_vecAbsOrigin = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("F3 0F 7E 97"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 4));
    });
    LogE("m_vecAbsOrigin : 0x%llx", m_vecAbsOrigin);
    if (!m_vecAbsOrigin) {
        errors.push_back("m_vecAbsOrigin not found");
    } else {
        output["m_vecAbsOrigin"] = m_vecAbsOrigin;
    }

    // [SIG] camera_origin - 相机坐标偏移(从player对象读)
    // 反汇编: ucomiss xmm1, [rcx+cameraOff]  ; 0F 2E 89 xx xx xx xx
    //         jp +0x62                       ; 7A 62
    // proc返回 addr+3 处的disp32 = player对象里 camera_origin 字段偏移
    // 如何找到: [无直接字符串引用] 函数无调用者xref, 通过代码流入, 特征是ucomiss+jp做NaN检查
    uint64_t camera_origin = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("0F 2E 89 ? ? ? ? 7A 62"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("camera_origin : 0x%llx", camera_origin);
    if (!camera_origin) {
        errors.push_back("camera_origin not found");
    } else {
        output["camera_origin"] = camera_origin;
    }

    // [SIG] commandNumber - 客户端命令计数器全局
    // 反汇编: xor r14d, r14d              ; 45 33 F6
    //         mov ebp, [rip+cmdNumber]    ; 8B 2D xx xx xx xx
    // 如何找到: 搜索字符串 "-NoQueuedPacketThread" → xref进入函数 → 该函数底部调用 cl_move
    //   cl_move 内部调用本函数(sub_7FF72CA30500), sig在本函数开头附近
    //   路径: "-NoQueuedPacketThread"函数 → cl_move → 本函数
    // RVA=9: 解析 mov ebp,[rip+X] 指令(从匹配起始+3开始的6字节指令)
    uint64_t commandNumber = (uintptr_t)(Pattern::FindPattern(ctx.data, ("45 33 F6 8B 2D ? ? ? ?"), 9));
    LogE("commandNumber : 0x%llx", commandNumber);
    if (!commandNumber) {
        errors.push_back("commandNumber not found");
    } else {
        output["commandNumber"] = commandNumber;
    }

    // [SIG] cinput - CInput全局实例指针
    // 反汇编: lea rcx, [rip+CInput]  ; 48 8D 0D xx xx xx xx
    //         xor edx, edx           ; 33 D2
    //         add rsp, 0x20          ; 48 83 C4 20  (函数尾声)
    // 如何找到: [无直接字符串引用] 函数是.rdata vtable中的第4个条目, 无字符串引用链
    uintptr_t cinput = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 8D 0D ? ? ? ? 33 D2 48 83 C4 20"), 7));
    LogE("cinput : 0x%llx", cinput);
    if (!cinput) {
        errors.push_back("cinput not found");
    } else {
        output["cinput"] = cinput;
    }

    // [SIG] CHLClient - 客户端接口实例指针
    // 反汇编: mov rcx, [rip+CHLClient]  ; 48 8B 0D xx xx xx xx
    //         mov rsi, [rsp+0x50]       ; 48 8B 74 24 50
    // 有多个匹配(3个), FindPattern返回第一个
    // 如何找到: 搜索字符串 "-buildcubemaps" 或 "-navanalyze" → xref进入函数
    //   该函数是客户端帧处理函数, 开头处理命令行参数(-buildcubemaps等)
    //   CHLClient 的 mov rcx,[rip+X] 在函数中间, 用于获取客户端接口
    //   或搜: "nav_edit 1;nav_analyze_scripted\n" / "-exit" / "quit\n" → 同一个函数
    uintptr_t CHLClient = (Pattern::FindPattern<uintptr_t>(ctx.data, ("48 8B 0D ? ? ? ? 48 8B 74 24 50"), 7));
    LogE("CHLClient :%llx", CHLClient);
    if (!CHLClient) {
        errors.push_back("CHLClient not found");
    } else {
        output["CHLClient"] = CHLClient;
    }

    // [SIG] observerList - 观察者列表全局
    // 反汇编: mov rcx, [rip+observerList]  ; 48 8B 0D xx xx xx xx
    //         test rcx, rcx                 ; 48 85 C9
    //         jz +?                         ; 74 ?
    //         mov rax, [rcx]                ; 48 8B 01
    //         call qword ptr [rax+?]        ; FF ?? ??
    //         test rax, rax                 ; 48 85 C0
    //         jz +?                         ; 74 ?
    //         movsxd rcx, [rsi+0x38]        ; 48 63 4E 38  (读观察者index)
    // 如何找到: 搜索字符串 "lastVisibleTime" → xref进入大函数(~0x2156字节)
    //   该大函数调用本函数(sub_7FF72CFA3750, observer lookup), sig在本函数内
    //   下一个sig observer_index 也在同一个函数里
    uintptr_t observerList = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF ? ? 48 85 C0 74 ? 48 63 4E 38"), 7));
    LogE("observerList : 0x%llx", observerList);
    if (!observerList) {
        errors.push_back("observerList not found");
    } else {
        output["observerList"] = observerList;
    }

    // [SIG] observer_index - 观察者索引数组的偏移
    // 反汇编: mov eax, [rax+rcx*8+observerIdxOff]  ; 8B 84 C8 xx xx xx xx
    //         cmp eax, imm                          ; 83 F8 ??
    // proc返回 addr+3 处的disp32 = 观察者index数组在某个对象里的偏移
    // 如何找到: 与observerList在同一个函数里, 紧跟observerList加载之后的索引计算
    //   路径同上: "lastVisibleTime" → 大函数 → 本函数(sub_7FF72CFA3750)
    uint64_t observer_index = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("8B 84 C8 ? ? ? ? 83 F8"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("observer_index : 0x%llx", observer_index);
    if (!observer_index) {
        errors.push_back("observer_index not found");
    } else {
        output["observer_index"] = observer_index;
    }

    // [SIG] netChannel - 客户端网络通道指针
    // 反汇编: 48 ?? ?? ?? ?? ?? ?? (某个48前缀的7字节指令, 通常是lea/mov rcx)
    //         call sub_XXX            ; E8 xx xx xx xx
    //         test al, al             ; 84 C0
    //         jnz +?                  ; 75 ?
    //         movzx esi, r15b/dil     ; 40 0F B6 F7
    // 如何找到: 搜索字符串 "-NoQueuedPacketThread" → xref进入函数 → 该函数底部调用 cl_move
    //   sig就在 cl_move 函数体内
    //   IDA里 cl_move 可能已被命名(不是 sub_XXX)
    uintptr_t netChannel = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 ? ? ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 40 0F B6 F7"), 7));
    LogE("netChannel : %llx", netChannel);
    if (netChannel == NULL) {
        errors.push_back("netChannel not found");
    } else {
        output["netChannel"] = netChannel;
    }


    uintptr_t ClientState = Pattern::FindPatternByProc<uintptr_t>(ctx.data,("E8 ? ? ? ? 48 81 C4 68 04 00 00 C3"), [&](uint64_t addr, uint64_t base)->uint64_t {
        return (uint64_t)(RVA(addr - 7, 7) - (uintptr_t)base);
    });
    LogE("ClientState : %llx", ClientState);
    if (ClientState == NULL) {
        errors.push_back("ClientState not found");
    } else {
        output["ClientState"] = ClientState;
    }

    // [SIG] SignonState - 客户端signon状态全局字节
    // 反汇编: mov byte [rcx+0x14], 1    ; C6 41 14 01
    //         cmp byte [rip+?], 0       ; 80 3D xx xx xx xx 00
    //         jz +0x68                  ; 74 68
    //         cmp byte [rip+?], 0       ; 80 3D xx xx xx xx 00
    //         jnz +0x09                 ; 75 09
    //         cmp dword [rip+SignonState], ?  ; 83 3D xx xx xx xx
    // RVA=28: 解析第4条 cmp dword [rip+X] 指令的rip相对地址
    //   代码里随后 SignonState += 1 修正偏移到状态字段
    // 如何找到: [无直接字符串引用] 函数很小(0xbf字节), 被29个地方调用
    //   是网络序列化的位写入函数, 调用链涉及大量网络编码逻辑, 均无字符串
    uintptr_t SignonState = Pattern::FindPattern<uintptr_t>(ctx.data, ("C6 41 14 01 80 3D ? ? ? ? ? 74 68 80 3D ? ? ? ? ? 75 09 83 3D"), 28);
    LogE("SignonState : %llx", SignonState);
    if (SignonState == NULL) {
        errors.push_back("SignonState not found");
    } else {
        SignonState += 1;
        output["SignonState"] = SignonState;
    }

    // [SIG] localplayerHandle - 本地玩家handle全局
    // 反汇编: movaps [rax+?], xmm9   ; 44 0F 29 48 ??    (保存寄存器)
    //         mov eax, [rip+handle]  ; 8B 05 xx xx xx xx
    //         cmp eax, -1            ; 83 F8 FF
    // RVA=11 解析第2条 mov eax,[rip+X] 得到handle地址
    // 如何找到: [无直接字符串引用] 函数通过代码流进入(上一个函数ret后紧跟), 无调用者xref
    uintptr_t localplayerHandle = (uintptr_t)(Pattern::FindPattern(ctx.data, ("44 0F 29 48 ? 8B 05 ? ? ? ? 83 F8 FF"), 11));
    LogE("localplayerHandle : %llx", localplayerHandle);
    if (localplayerHandle == NULL) {
        errors.push_back("localplayerHandle not found");
    } else {
        output["localplayerHandle"] = localplayerHandle;
    }

    // [SIG] WeaponNames - 武器名字表全局指针(通过虚调用+0x58访问)
    // 反汇编: jz +0x12                     ; 74 12
    //         mov rcx, [rip+WeaponNames]   ; 48 8B 0D xx xx xx xx
    //         mov rax, [rcx]               ; 48 8B 01
    //         call qword ptr [rax+0x58]    ; FF 50 58
    // RVA=9 解析第2条指令 mov rcx,[rip+X]
    // 如何找到: 搜索字符串 "Weapon %s not precached; cannot assign to projectile" → xref进入函数
    //   sig的 jz+mov rcx 就在该字符串引用的上方几行
    //   sig有2个匹配, FindPattern取第一个
    uintptr_t WeaponNames = Pattern::FindPattern<uintptr_t>(ctx.data, ("74 12 48 8B 0D ? ? ? ? 48 8B 01 FF 50 58"), 9);
    LogE("WeaponNames : 0x%p", WeaponNames);
    if (WeaponNames == NULL) {
        errors.push_back("WeaponNames not found");
    } else {
        output["WeaponNames"] = WeaponNames;
    }

    // [SIG] ModelNames - 模型名字表全局指针
    // 反汇编: mov rcx, [rip+ModelNames]  ; 48 8B 0D xx xx xx xx
    //         test rcx, rcx              ; 48 85 C9
    //         jz +0x0F                   ; 74 0F
    //         lea eax, [rdx+1]           ; 8D 42 01
    // 如何找到: [无直接字符串引用] 极小函数(0x22字节), 是vtable条目, 无字符串引用链
    uintptr_t ModelNames = Pattern::FindPattern<uintptr_t>(ctx.data, ("48 8B 0D ? ? ? ? 48 85 C9 74 0F 8D 42 01"), 7);
    LogE("ModelNames : 0x%p", ModelNames);
    if (ModelNames == NULL) {
        errors.push_back("ModelNames not found");
    } else {
        output["ModelNames"] = ModelNames;
    }

    return true;
}
