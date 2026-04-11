#include "weaponSettings.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>

struct RawWeaponDataField {
    uint64_t name;
    uint64_t default_val;
    uint64_t description;
    uint32_t unk1;
    uint32_t offset;
};

static_assert(sizeof(RawWeaponDataField) == 32, "RawWeaponDataField size must be 32 bytes");

bool weaponSettings::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errors) {
    std::uint64_t weaponSettingsArr = Pattern::FindPattern<std::uint64_t>(ctx.data, ("48 8D 05 ? ? ? ? 44 0F B6 44 24 ?"), 7);

    uint64_t weaponSettingsArrSize = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("41 8D 40 FF 3D ? ? ? ? 0F 87 ? ? ? ?"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 5));
    });
    if (!weaponSettingsArrSize) {
        errors.push_back("weaponSettingsArrSize not found");
        return false;
    }

    LogE("weaponSettingsArr %p", weaponSettingsArr);
    if (!weaponSettingsArr) {
        errors.push_back("weaponSettingsArr not found");
        return false;
    }

    for (int i = 0; i < weaponSettingsArrSize * 2; ++i) {
        readEntity(ctx, output, (uint64_t)(ctx.data.data() + weaponSettingsArr + i * sizeof(RawWeaponDataField)));
    }

    return true;
}

void weaponSettings::readEntity(const dumpContext& ctx, std::map<std::string, uint64_t>& items, uint64_t addr) {
    auto WeaponDataField = reinterpret_cast<RawWeaponDataField*>(addr);

    if (!PS::In(ctx.baseAddress, ctx.data.size(), WeaponDataField->name, 8)) {
        return;
    }

    auto weaponName = reinterpret_cast<char*>((char*)(ctx.data.data() + WeaponDataField->name - ctx.baseAddress));
    if (!PS::isAsciiOnly(weaponName)) return;

    items[weaponName] = WeaponDataField->offset;
}
