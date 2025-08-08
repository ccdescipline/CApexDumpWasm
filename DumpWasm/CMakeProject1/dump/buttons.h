//
// Created by admin on 2025/4/29.
//

#ifndef CMAKEPROJECT1_BUTTONS_H
#define CMAKEPROJECT1_BUTTONS_H

#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../NT/NTHeader.h"
#include "../3rd/Log.h"

class buttons{
public:
    struct RawConCommand
    {
        uint64_t m_vTable;               // 0x0000
        uint64_t m_pNext;                // 0x0008
//        bool     m_bRegistered;          // 0x0010
//        char     pad_0011[7];            // 0x0011
        uint64_t m_pszName;              // 0x0018
        uint64_t m_pszDescription;       // 0x0020
        uint64_t m_pszDataType;          // 0x0028
        uint32_t m_iFlags;               // 0x0030
        char     pad_0034[20];           // 0x0034
        uint64_t m_fnCommandCallback;    // 0x0048
        uint64_t m_fnCompletionCallback; // 0x0050
        uint32_t m_fnCommandType;        // 0x0058
    };

    inline static bool dump(dumpContext ctx,std::map<std::string, uint64_t>& output){
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
            if ( !conc->m_pszName || !conc->m_fnCommandCallback || conc->m_fnCommandType != 2) continue;



            //char name[33];
            //name[sizeof(name) - 1] = '\0';
            //if (!instance->RPM(conc->m_pszName, name, sizeof(name) - 1)) continue;
            if (!conc->m_pszName || !PS::In(ctx.baseAddress,ctx.data.size(),conc->m_pszName,8)) {
                continue;
            }
            std::string name = std::string((char *)(ctx.data.data() + conc->m_pszName - ctx.baseAddress));
            if (!name.size() || name[0] != '+') continue;

            //LogE("name: %s", name.c_str());

            auto cstr = std::string((char *)(ctx.data.data() + (conc->m_fnCommandCallback - ctx.baseAddress)), 0x100);
            //cstr.resize(0x100);
            //if (!instance->RPM(, cstr.data(), cstr.size())) continue;

            auto offset = PS::Search(cstr.c_str(), cstr.size(),
                                     ("\x84\xC0\x75\x44\x8B\x05\x00\x00\x00\x00\x3B\xD8\x74\x3A\x8B\x0D\x00\x00\x00\x00\x3B\xD9\x74\x30\x85\xC0\x75\x08"), ("xxxxxx????xxxxxx????xxxxxxxx"));
            if (!offset) continue;
            offset += 2 + 2 + 2;
            offset += *reinterpret_cast<const int32_t*>(cstr.data() + offset);
            offset += 4;
            offset += conc->m_fnCommandCallback - ctx.baseAddress;

            offsets[name] = offset;

            //std::cout <<"Key: " << name << " Value: " << offset << std::endl;
            //LogE("Key: %s Value: %p", name.c_str(), offset);
        }

        output = offsets;

        return true;
    }

};

#endif //CMAKEPROJECT1_BUTTONS_H
