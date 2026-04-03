#include "buttons.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>

bool buttons::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output) {
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

    auto matches = PS::SearchInSectionMultiple(ctx.data.data(), (".data"),
        ("\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00\x00\x00\x00\x00\x00\x7F\x00\x00"),
        ("xxx?????xxx?????xxx?????xxx"));

    if (!matches.size()) {
        std::cout << "button matches Null" << std::endl;
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
