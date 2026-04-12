#include "buttons.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>

bool buttons::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errors) {
    struct RawConCommand
    {
        uint64_t m_vTable;
        uint64_t m_pNext;
        uint64_t m_pszName;
        uint64_t m_pszDescription;
        uint64_t m_pszDataType;
        uint32_t m_iFlags;
        char     pad_0034[20];
        uint64_t m_fnCommandCallback;
        uint64_t m_fnCompletionCallback;
        uint32_t m_fnCommandType;
    };

    // [SIG] ConCommand数据模式 - 在.data段匹配4个连续用户态指针的高字节(byte5,6,7 = 0x7F,0x00,0x00)
    // 如何找到: 搜索字符串 "+forward" 或 "+attack" → xref到.data段 → 那就是ConCommand结构体
    //   往上看16字节(offset 0x00)就是结构体起始, 第一个qword是vtable
    //   vtable xref进入的是ConCommand构造函数, 里面设置:
    //     *(_QWORD *)a1 = off_XXX;           // vtable
    //     *(_QWORD *)(a1 + 16) = a2;         // pszName
    //     *(_QWORD *)(a1 + 64) = a3;         // fnCommandCallback
    //   结构体: vTable(0x00) pNext(0x08) pszName(0x10) pszDesc(0x18) pszDataType(0x20)
    //           iFlags(0x28) pad(0x2C,20B) fnCallback(0x40) fnCompletion(0x48) fnType(0x50)
    // 过滤条件: pszName以'+'开头, fnCommandType==2
    auto matches = PS::SearchInSectionMultiple(ctx.data.data(), (".data"),
        ("\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00"),
        ("xxx?????xxx?????xxx?????xxx"));

    if (!matches.size()) {
        LogE("button matches Null");
        errors.push_back("ConCommand signature pattern not found");
        return false;
    };

    auto offsets = std::map<std::string, uint64_t>();
    for (auto i = size_t(); i < matches.size(); i++)
    {
        auto conc = reinterpret_cast<const RawConCommand*>(ctx.data.data() + matches[i] - 5);
        if (!conc->m_pszName || !conc->m_fnCommandCallback || conc->m_fnCommandType != 2) continue;

        if (!conc->m_pszName || !PS::In(ctx.baseAddress, ctx.data.size(), conc->m_pszName, 8)) {
            continue;
        }
        std::string name = std::string((char*)(ctx.data.data() + conc->m_pszName - ctx.baseAddress));
        if (!name.size() || name[0] != '+') continue;

        auto cstr = std::string((char*)(ctx.data.data() + (conc->m_fnCommandCallback - ctx.baseAddress)), 0x100);

        // [SIG] Button offset提取 - 在ConCommand回调函数(fnCallback, offset 0x40)体内搜索
        // 反汇编: test al,al / jnz 44h / mov eax,[rip+X] / cmp ebx,eax / jz 3Ah
        //         / mov ecx,[rip+Y] / cmp ebx,ecx / jz 30h / test eax,eax / jnz 08h
        // 如何找到: 从ConCommand结构体取fnCommandCallback(offset 0x40)进入回调函数
        //   函数开头附近有 test al,al 后跟 mov eax,[rip+X] 读取按钮状态变量
        //   X处的rip相对地址就是按钮offset
        auto offset = PS::Search(cstr.c_str(), cstr.size(),
            ("\x84\xC0\x75\x44\x8B\x05\x00\x00\x00\x00\x3B\xD8\x74\x3A\x8B\x0D\x00\x00\x00\x00\x3B\xD9\x74\x30\x85\xC0\x75\x08"),
            ("xxxxxx????xxxxxx????xxxxxxxx"));
        if (!offset) continue;
        offset += 2 + 2 + 2;
        offset += *reinterpret_cast<const int32_t*>(cstr.data() + offset);
        offset += 4;
        offset += conc->m_fnCommandCallback - ctx.baseAddress;

        offsets[name] = offset;
    }

    output = offsets;
    return true;
}
