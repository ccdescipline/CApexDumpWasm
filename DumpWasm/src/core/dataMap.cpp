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
