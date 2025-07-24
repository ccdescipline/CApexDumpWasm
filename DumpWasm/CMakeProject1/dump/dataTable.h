//
// Created by admin on 2025/4/28.
//

#ifndef CMAKEPROJECT1_DATATABLE_H
#define CMAKEPROJECT1_DATATABLE_H

#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../NT/NTHeader.h"

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


class dataTable{

public:

    inline static std::map<std::string, std::map<std::string, uint64_t>>  dataTableOffsets = std::map<std::string, std::map<std::string, uint64_t>>();



    inline static bool dump(dumpContext ctx,std::map<std::string, std::map<std::string, uint64_t>>& output){
        auto offsets = std::map<std::string, std::map<std::string, uint64_t>>();
        {

            auto matches = PS::SearchInSectionMultiple(ctx.data.data(),".text","\x4C\x8D\x0D\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8","xxx????xx????xxx????xxx????x");

            int matchesCount = 0;
            if (!matches.size()) return false;

            for (auto i = size_t(); i < matches.size(); i++)
            {
                auto tableBase = reinterpret_cast<const RecvTable*>( (uint64_t)RVA(matches[i] + 7 + 6 + 7+ ctx.data.data(), 7));
                auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));
                auto tableProp = reinterpret_cast<const RecvProp*>( RVA(matches[i] + 7 + 6+ ctx.data.data(), 7));

                for (auto x = 0; x < tableBase->m_iProps; x++)
                {
                    //if (!info->Is(tableProp[x].m_name)) continue;
                    auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);

                    offsets[tableName][name] = tableProp[x].m_offset;
                    matchesCount++;
                }
            }

            std::cout << "parrten1 table count " <<  matchesCount << std::endl;
        }

        {
            auto matches = PS::SearchInSectionMultiple(ctx.data.data(),".text",
                                                       "\x48\x8D\x15\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8"
                    ,"xxx????xx????xxx????x");
            int matchesCount = 0;
            if (!matches.size()) return false;

            for (auto i = size_t(0); i < matches.size(); i++)
            {
                auto tableBase = reinterpret_cast<const RecvTable*>(  RVA(matches[i] + 7 + 6+ctx.data.data(), 7));
                auto tableProp = reinterpret_cast<const RecvProp*>( RVA(matches[i] + ctx.data.data(), 7));
                if (!PS::In(ctx.baseAddress,ctx.data.size(),tableBase->m_name,8)) continue;
                auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));

                //std::cout<< "tableName " << tableName << std::endl;

                for (auto x = 0; x < tableBase->m_iProps; x++)
                {
//                    if (!info->Is(tableProp[x].m_name)) continue;
                    auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);

                    offsets[tableName][name] = tableProp[x].m_offset;
                    matchesCount++;
                }
            }
            std::cout << "parrten2 table count " <<  matchesCount << std::endl;
        }

        {
            auto matches = PS::SearchInSectionMultiple(ctx.data.data(),".text",
                                                       "\x48\x89\x0D\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x48"
                    ,"xxx????xxx????xx????????x");

            int matchesCount = 0;
            if (!matches.size()) return false;

            for (auto i = size_t(0); i < matches.size(); i++)
            {
                auto tableBase = reinterpret_cast<const RecvTable*>(RVA(matches[i] + 7 + 7 +ctx.data.data(), 6) + 4 - 16);
                auto tableProp = reinterpret_cast<const RecvProp*>(RVA(matches[i] + 7 + ctx.data.data(), 7));
                if (!PS::In(ctx.baseAddress,ctx.data.size(),tableBase->m_name,8)) continue;
                auto tableName = reinterpret_cast<const char*>(ctx.data.data() + (tableBase->m_name - ctx.baseAddress));

                for (auto x = 0; x < tableBase->m_iProps; x++)
                {
                    //if (!info->Is(tableProp[x].m_name)) continue;
                    auto name = ctx.data.data() + (tableProp[x].m_name - ctx.baseAddress);

                    offsets[tableName][name] = tableProp[x].m_offset;
                    matchesCount++;
                }
            }
            std::cout << "parrten3 table count " <<  matchesCount << std::endl;
        }

        output = offsets;
        return true;
    }


};

#endif //CMAKEPROJECT1_DATATABLE_H
