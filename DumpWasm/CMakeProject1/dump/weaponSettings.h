//
// Created by admin on 2025/5/7.
//

#ifndef CMAKEPROJECT1_WEAPONSETTINGS_H
#define CMAKEPROJECT1_WEAPONSETTINGS_H

#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../3rd/Log.h"

struct RawWeaponDataField {
    uint64_t name;     // Ptr<CStr>
    uint64_t default_val; // default 是 C 关键字，避免冲突
    uint64_t description;
    uint32_t unk1;
    uint32_t offset;
//    uint8_t ty;
//    uint8_t flags;
//    uint16_t index;
//    uint16_t offset;
//    uint8_t pad[2];       // 保持对齐到 32 字节
};

_Static_assert(sizeof(RawWeaponDataField) == 32, "RawWeaponDataField size must be 32 bytes");

class weaponSettings {
public:


    inline static bool dump(dumpContext ctx, std::map<std::string, uint64_t> &output) {


        std::uint64_t weaponSettingsArr = Pattern::FindPattern<std::uint64_t>(ctx.data,("48 8D 05 ? ? ? ? 44 0F B6 44 24 ?"), 7);

        uint64_t weaponSettingsArrSize = Pattern::FindPatternByProc<uint64_t>(ctx.data,("41 8D 40 FF 3D ? ? ? ? 0F 87 ? ? ? ?"), [&](uint64_t addr, uint64_t base)->uint64_t {
            return (uint64_t)(*(UINT32*)( (uint64_t)addr + 5));
        });
        if (!weaponSettingsArrSize) {
            return false;
        }

        LogE("weaponSettingsArr %p", weaponSettingsArr);
        if (!weaponSettingsArr) {
            return false;
        }

        for (int i = 0; i < weaponSettingsArrSize * 2; ++i) {
            readEntity(ctx, output, (uint64_t)(ctx.data.data() + weaponSettingsArr + i * sizeof(RawWeaponDataField)));
        }


        return true;
    }

    inline static void readEntity(dumpContext& ctx,std::map<std::string, uint64_t>& items,uint64_t addr){
        auto WeaponDataField =  reinterpret_cast<RawWeaponDataField*>(addr);

        if(!PS::In(ctx.baseAddress,ctx.data.size(),WeaponDataField->name,8)  ){
            return;
        }

        auto weaponName = reinterpret_cast<char *>((char *)(ctx.data.data() +  WeaponDataField->name - ctx.baseAddress));
        if(!PS::isAsciiOnly(weaponName))  return ;

        items[weaponName] = WeaponDataField->offset;

        //LogE("name : %s offset 0x%x ",(ctx.data.data() +  WeaponDataField->name - ctx.baseAddress),WeaponDataField->offset);
    }
};

#endif //CMAKEPROJECT1_WEAPONSETTINGS_H
