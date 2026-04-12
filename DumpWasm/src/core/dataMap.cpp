#include "dataMap.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>
#include <cstring>

bool dataMap::dump(const dumpContext& ctx, std::map<std::string, std::map<std::string, uint64_t>>& output, std::vector<std::string>& errors) {
    struct DataMap
    {
        uint64_t m_dataDesc;
        uint32_t m_dataNumFields;
        uint32_t _pad0;
        uint64_t m_dataClassName;
        uint64_t _pad1;
        uint64_t _pad2;
        uint64_t m_baseMap;
    };

    struct DataTypeDesc
    {
        int32_t  m_fieldType;
        uint32_t _pad0;
        uint64_t m_fieldName;
        uint32_t m_fieldOffset[2];
        uint64_t m_externalName;
        uint64_t _pad1[5];
        uint64_t m_td;
        int32_t  m_fieldSizeInBytes;
        uint64_t _pad2[5];
    };

    // [SIG] DataMap getter - 匹配所有 "lea rax,[rip+X]; ret" 的微型getter函数
    // 这是个极其通用的模式(会匹配上千个函数), 靠下面的DataMap结构体验证来过滤:
    //   - m_dataNumFields(offset 0x08) 在 1~0x1000 范围内
    //   - m_dataClassName(offset 0x10) 是有效的ASCII字符串指针
    //   - m_dataDesc(offset 0x00) 是有效的DataTypeDesc数组指针
    // 如何找到: 搜索实体类名如 "C_Player" → xref到.data段 → 存储位置即DataMap.m_dataClassName(+0x10)
    //   往前0x10就是DataMap结构体起始, 再xref这个地址 → 就能找到对应的getter函数(lea rax,[DataMap]; ret)
    // DataMap结构: m_dataDesc(0x00,ptr) m_dataNumFields(0x08,u32) m_dataClassName(0x10,ptr)
    //              _pad(0x18,0x20) m_baseMap(0x28,ptr)
    auto matches = PS::SearchInSectionMultiple(ctx.data.data(), ".text", "\x48\x8D\x05\x00\x00\x00\x00\xC3", "xxx????x");

    if (!matches.size()) {
        LogE("DataMap matches Null");
        errors.push_back("DataMap signature pattern not found");
    }

    for (auto i = size_t(); i < matches.size(); i++)
    {
        auto addr = matches[i];
        addr += 3;
        addr += *reinterpret_cast<const std::int32_t*>(ctx.data.data() + addr);
        addr += 4;

        if (addr < 0x13d2000 && PS::In(ctx.baseAddress, ctx.data.size(), (uint64_t)(ctx.data.data() + addr), 8)) {
            continue;
        }

        auto maps = reinterpret_cast<const DataMap*>(ctx.data.data() + addr);

        if (!maps->m_dataNumFields || maps->m_dataNumFields < 0 || maps->m_dataNumFields > 0x1000) continue;

        if (!PS::In(ctx.baseAddress, ctx.data.size(), (uint64_t)(maps->m_dataClassName), 8)) {
            continue;
        }
        auto cstr = (char*)(maps->m_dataClassName - ctx.baseAddress + ctx.data.data());
        if (!std::strlen(cstr) || !PS::isAsciiOnly(cstr)) continue;

        if (!PS::In(ctx.baseAddress, ctx.data.size(), (uint64_t)(maps->m_dataDesc), 8)) {
            continue;
        }
        auto desc = reinterpret_cast<const DataTypeDesc*>(maps->m_dataDesc - ctx.baseAddress + ctx.data.data());

        for (auto j = 0u; j < maps->m_dataNumFields; j++)
        {
            if (!PS::In(ctx.baseAddress, ctx.data.size(), (uint64_t)(desc[j].m_fieldName), 8)) {
                continue;
            }

            auto name = reinterpret_cast<const char*>(desc[j].m_fieldName - ctx.baseAddress + ctx.data.data());
            if (!std::strlen(name) || !PS::isAsciiOnly(name)) continue;

            output[cstr][name] = desc[j].m_fieldOffset[0];
        }
    }
    return true;
}
