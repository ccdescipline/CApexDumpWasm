#include "dataTable.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>

struct RecvTable
{
    uint64_t _pad0;
    uint64_t m_pProps;
    int32_t  m_iProps;
    uint32_t _pad1[256];
    uint32_t _pad2[43];
    uint64_t m_decoder;
    uint64_t m_name;
    bool     m_initialized;
    bool     m_inMainList;
    uint8_t  _pad3[6];
};

struct RecvProp
{
    int32_t  m_type;
    int32_t  m_offset;
    uint32_t _pad0[6];
    uint64_t m_dataTable;
    uint64_t m_name;
    bool     m_isInsideArray;
    uint8_t  _pad1[7];
    uint64_t m_arrayProp;
    uint64_t m_proxyFn;
    uint32_t _pad2[4];
    int32_t  m_flags;
    uint32_t _pad3;
    int32_t  m_iNumElements;
    uint32_t _pad4;
};

bool dataTable::dump(const dumpContext& ctx, std::map<std::string, std::map<std::string, uint64_t>>& output, std::vector<std::string>& errors) {
    // [SIG] RecvTable注册 pattern1 - 完整4参数的RecvTable构造函数调用
    // 反汇编:
    //   lea r9,  [rip+name]        ; 4C 8D 0D xx xx xx xx   (name字符串)
    //   mov r8d, numProps          ; 41 B8 xx xx xx xx      (RecvProp数量)
    //   lea rdx, [rip+RecvProps]   ; 48 8D 15 xx xx xx xx   (RecvProp数组地址)
    //   lea rcx, [rip+RecvTable]   ; 48 8D 0D xx xx xx xx   (RecvTable对象地址)
    //   call RecvTable::RecvTable  ; E8 xx xx xx xx
    // MSVC x64调用约定: rcx=RecvTable*, rdx=RecvProp*, r8d=numProps, r9=name
    // 如何找到: 搜索 "DT_Player" / "DT_BaseEntity" 等DT_前缀的表名字符串
    //   xref到 lea rXX, [aDtXxx] 的地方 → 附近就是RecvTable注册代码
    //   call目标就是RecvTable构造函数 sub_XXX(RecvTable*, RecvProp*, numProps, name)
    //   构造函数内部会设置: a1[1]=propsBuf, a1[16]=numProps, a1[0x4C8]=name
    // 代码解析:
    //   tableBase (RecvTable*) 通过解析 lea rcx (第4条指令, matches[i]+7+6+7, RVA=7)
    //   tableProp (RecvProp*)  通过解析 lea rdx (第3条指令, matches[i]+7+6,   RVA=7)
    {
        auto matches = PS::SearchInSectionMultiple(ctx.data.data(), ".text",
            "\x4C\x8D\x0D\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8",
            "xxx????xx????xxx????xxx????x");

        int matchesCount = 0;
        if (!matches.size()) {
            LogE("parrten1 matches Null");
            errors.push_back("pattern1 no matches");
        }

        for (auto i = size_t(); i < matches.size(); i++)
        {
            auto tableBase = reinterpret_cast<const RecvTable*>((uint64_t)RVA(matches[i] + 7 + 6 + 7 + ctx.data.data(), 7));
            auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));
            auto tableProp = reinterpret_cast<const RecvProp*>(RVA(matches[i] + 7 + 6 + ctx.data.data(), 7));

            if (!PS::In(ctx.baseAddress, ctx.data.size(), tableBase->m_name, 8)) continue;

            if (!PS::isAsciiOnly(tableName)) {
                continue;
            }

            for (auto x = 0; x < tableBase->m_iProps; x++)
            {
                if (!PS::In(ctx.baseAddress, ctx.data.size(), tableProp[x].m_name, 8)) {
                    continue;
                }
                auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);
                if (!PS::isAsciiOnly(name)) continue;

                output[tableName][name] = tableProp[x].m_offset;
                matchesCount++;
            }
        }

        LogE("parrten1 table count %d", matchesCount);
    }

    // [SIG] RecvTable注册 pattern2 - 缺少 lea r9 的简化变体
    // 反汇编:
    //   lea rdx, [rip+RecvProps]   ; 48 8D 15 xx xx xx xx
    //   mov r8d, numProps          ; 41 B8 xx xx xx xx
    //   lea rcx, [rip+RecvTable]   ; 48 8D 0D xx xx xx xx
    //   call RecvTable::RecvTable  ; E8 xx xx xx xx
    // 与pattern1区别: 没有 lea r9,[name] - r9(name参数)来自其他来源(上游寄存器/栈)
    //   例如前面已经 mov r9, rdi 或类似指令加载过name
    // 如何找到: 同pattern1, 搜索DT_前缀字符串xref, 当编译器选择用非lea方式传name时命中此pattern
    // 代码解析:
    //   tableBase 通过解析 lea rcx (第3条指令, matches[i]+7+6, RVA=7)
    //   tableProp 通过解析 lea rdx (第1条指令, matches[i],     RVA=7)
    {
        auto matches = PS::SearchInSectionMultiple(ctx.data.data(), ".text",
            "\x48\x8D\x15\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8",
            "xxx????xx????xxx????x");

        int matchesCount = 0;
        if (!matches.size()) {
            LogE("parrten2 matches Null");
            errors.push_back("pattern2 no matches");
        }

        for (auto i = size_t(0); i < matches.size(); i++)
        {
            auto tableBase = reinterpret_cast<const RecvTable*>(RVA(matches[i] + 7 + 6 + ctx.data.data(), 7));
            auto tableProp = reinterpret_cast<const RecvProp*>(RVA(matches[i] + ctx.data.data(), 7));
            if (!PS::In(ctx.baseAddress, ctx.data.size(), tableBase->m_name, 8)) continue;
            auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));
            if (!PS::isAsciiOnly(tableName)) {
                continue;
            }

            for (auto x = 0; x < tableBase->m_iProps; x++)
            {
                if (!PS::In(ctx.baseAddress, ctx.data.size(), tableProp[x].m_name, 8)) {
                    continue;
                }
                auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);
                if (!PS::isAsciiOnly(name)) continue;

                output[tableName][name] = tableProp[x].m_offset;
                matchesCount++;
            }
        }
        LogE("parrten2 table count %d", matchesCount);
    }

    // [SIG] RecvTable注册 pattern3 - 内联初始化(不走构造函数调用)
    // 反汇编:
    //   mov [rip+X], rcx           ; 48 89 0D xx xx xx xx   (存入某个全局)
    //   lea rcx, [rip+RecvProps]   ; 48 8D 0D xx xx xx xx   (加载RecvProp数组)
    //   mov dword [rip+X], numProps; C7 05 xx xx xx xx nn nn nn nn
    //   ...                        ; 48 (下一条指令)
    // 这是编译器把RecvTable构造函数inline展开后的形态: 直接用mov指令初始化全局RecvTable字段
    // 如何找到: 搜索DT_前缀字符串xref, 当编译器inline了RecvTable ctor时命中
    //   通常m_iProps字段是 C7 05 (mov dword imm32) 立即数赋值, 是识别的关键特征
    // 代码解析:
    //   tableProp 通过解析 lea rcx (第2条指令, matches[i]+7, RVA=7)
    //   tableBase 通过 C7 05 (第3条指令, matches[i]+7+7) 的rip相对地址
    //              + 4(imm32跳过) - 16(m_iProps在RecvTable中相对start的负偏移调整)
    {
        auto matches = PS::SearchInSectionMultiple(ctx.data.data(), ".text",
            "\x48\x89\x0D\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x48",
            "xxx????xxx????xx????????x");

        int matchesCount = 0;
        if (!matches.size()) {
            LogE("parrten3 matches Null");
            errors.push_back("pattern3 no matches");
        }

        for (auto i = size_t(0); i < matches.size(); i++)
        {
            auto tableBase = reinterpret_cast<const RecvTable*>(RVA(matches[i] + 7 + 7 + ctx.data.data(), 6) + 4 - 16);
            auto tableProp = reinterpret_cast<const RecvProp*>(RVA(matches[i] + 7 + ctx.data.data(), 7));
            if (!PS::In(ctx.baseAddress, ctx.data.size(), tableBase->m_name, 8)) continue;
            auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));
            if (!PS::isAsciiOnly(tableName)) {
                continue;
            }

            for (auto x = 0; x < tableBase->m_iProps; x++)
            {
                if (!PS::In(ctx.baseAddress, ctx.data.size(), tableProp[x].m_name, 8)) {
                    continue;
                }
                auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);
                if (!PS::isAsciiOnly(name)) continue;

                output[tableName][name] = tableProp[x].m_offset;
                matchesCount++;
            }
        }
        LogE("parrten3 table count %d", matchesCount);
    }

    return true;
}
