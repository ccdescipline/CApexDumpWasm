#include "Convar.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include <iostream>
#include <vector>

bool Convar::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errors) {
    struct RawConVar
    {
        std::uint64_t m_vTable;
        std::uint64_t m_pNext;
        std::uint64_t m_pszName;
        std::uint64_t m_pszDescription;
        std::uint64_t m_pszDataType;
        std::uint32_t m_iFlags;
        char     pad_0034[4];
        std::uint64_t m_pParent;
        std::uint64_t m_pTable;
        std::uint64_t m_pszDefaultValue;
        std::uint64_t m_pszString;
        std::uint64_t m_iStringLength;
        float    m_fValue;
        std::int32_t  m_nValue;
        bool     m_bHasMin;
        bool     m_bHasMax;
        char     pad_006A[2];
        float    m_fMinValue;
        float    m_fMaxValue;
    };

    auto offsets = std::map<std::string, uint64_t>();

    std::uint64_t conVarVtable = Pattern::FindPattern<std::uint64_t>(ctx.data, ("48 8B 79 ? 48 8D 05 ? ? ? ? 48 89 ? 48 8D"), 11);

    if (!conVarVtable) {
        LogE("can't find conVarVtable");
        errors.push_back("conVarVtable not found");
        return false;
    }

    conVarVtable = static_cast<std::uint64_t>(conVarVtable + ctx.baseAddress);

    auto convarbyte = [](std::uint64_t a1) {
        std::vector<uint8_t> res;
        for (int i = 0; i < sizeof(std::uint64_t); ++i) {
            uint8_t byte = (a1 >> (i * 8)) & 0xFF;
            res.push_back(byte);
        }
        return res;
    };

    auto matches = PS::SearchInSectionMultiple(ctx.data.c_str(), ".data", (const char*)convarbyte(conVarVtable).data(), "xxxxxxxx");

    if (!matches.size()) {
        LogE("matches Null");
        errors.push_back("no convar vtable references in .data");
        return false;
    }

    for (auto i = size_t(); i < matches.size(); i++) {
        auto offset = matches[i];
        RawConVar* convar = (RawConVar*)(ctx.data.data() + offset);

        if (!convar->m_pszDescription || !convar->m_pszName) {
            continue;
        }

        std::string name = std::string((char*)convar->m_pszName - ctx.baseAddress + (std::uint64_t)ctx.data.data());

        offsets[name] = offset;
    }

    output = offsets;
    return true;
}
