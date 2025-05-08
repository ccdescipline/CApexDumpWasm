//
// Created by admin on 2025/4/29.
//

#ifndef CMAKEPROJECT1_DATAMAP_H
#define CMAKEPROJECT1_DATAMAP_H

#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../NT/NTHeader.h"

class dataMap{
public:
    inline  static bool dump(dumpContext ctx,std::map<std::string, std::map<std::string, uint64_t>>& output){
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

        if (!matches.size()) return false;

        //std::cout << "matches.size()" << matches.size() << std::endl;
        //LogE("matches %d", matches.size());

        for (auto i = size_t(); i < matches.size(); i++)
        {
            //auto addr = RVAEx(matches[i], data.c_str(), 7);

            auto addr = matches[i];
            addr += 3;
            addr += *reinterpret_cast<const std::int32_t*>(ctx.data.data() + addr);
            addr += 4;

            //LogE("addr %p", addr);

            //不要读text
            if (addr < 0x13d2000 && PS::In(ctx.baseAddress,ctx.data.size(),(uint64_t)(ctx.data.data() + addr),8)  ) {
                continue;
            }

            //std::cout << "addr " << std::hex << addr << std::endl;

            auto maps = reinterpret_cast<const DataMap*>(ctx.data.data() + addr);
            //if (!info->Is(maps->m_dataDesc, sizeof(DataTypeDesc) * maps->m_dataNumFields) || !info->Is(maps->m_dataClassName)) continue;

            //LogE("maps %p", maps);
            if (!maps->m_dataNumFields || maps->m_dataNumFields < 0 || maps->m_dataNumFields > 0x1000) continue;
            //if (PS::In(data.data(),data.size(), maps->m_dataDesc,)) {

            //}

            //std::cout << "maps->m_dataClassName " << std::hex << maps->m_dataClassName << std::endl;

            //auto cstr = data.c_str() + (maps->m_dataClassName - GlobalData::baseAddress);

            //LogE("cstr offset %p addr :%p", maps->m_dataClassName - GlobalData::baseAddress, maps->m_dataClassName);
            if (
                  !PS::In(ctx.baseAddress,ctx.data.size(),(uint64_t)( maps->m_dataClassName),8)
                  // !( maps->m_dataClassName > ctx.baseAddress && (maps->m_dataClassName < ctx.baseAddress + ctx.data.size()))
                  ) {
                continue;
            }
            auto cstr = (char*)(maps->m_dataClassName - ctx.baseAddress + ctx.data.data());
            //std::cout << "cstr " << cstr << std::endl;
            if (!std::strlen(cstr)) continue;



            //LogE("cstr %s", cstr);
            if(!PS::In(ctx.baseAddress,ctx.data.size(),(uint64_t)( maps->m_dataDesc),8)){
                continue;
            }
            auto desc = reinterpret_cast<const DataTypeDesc*>(maps->m_dataDesc - ctx.baseAddress + ctx.data.data());

            for (auto j = 0u; j < maps->m_dataNumFields; j++)
            {
                //if (!info->Is(desc[j].m_fieldName)) continue;
                //LogE("desc[j].m_fieldName %p", desc[j].m_fieldName);
                if (
                    !PS::In(ctx.baseAddress,ctx.data.size(),(uint64_t)( desc[j].m_fieldName),8)
                        ) {
                    continue;
                }

                auto name = (char*)(desc[j].m_fieldName  - ctx.baseAddress + ctx.data.data());
                if (!std::strlen(name)) continue;

                output[cstr][name] = desc[j].m_fieldOffset[0];
                //std::cout<< cstr << "." << name << " = " << std::hex << desc[j].m_fieldOffset[0] << std::endl;
                //LogE("%s.%s = 0x%04x", cstr, name, desc[j].m_fieldOffset[0]);
            }


            //LogE("\n");
        }
        return true;
    }
};

#endif //CMAKEPROJECT1_DATAMAP_H
