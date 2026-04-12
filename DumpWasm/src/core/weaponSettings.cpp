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
    // 两个sig都在同一个weapon setting分发函数里
    // 如何找到该函数: 搜索字符串 "Invalid weapon setting index #%d." 或
    //                "Weapon setting '%s' is type %s, not %s.\n" → xref进入函数
    // 该函数签名类似 sub_XXX(a1, a2, int a3, a4, a5), 开头代码:
    //   if ((unsigned int)(a3 - 1) > 0x4F7) { error("Invalid weapon setting index #%d."); return 0; }
    //   v5 = (char *)&weaponSettingsArr + 64 * a3;
    //   switch(v5[24]) { case 1: ... case 10: ... }

    // [SIG] weaponSettingsArr - 加载武器设置数组基址
    // 反汇编: lea rax, [rip+weaponSettingsArr]   ; 48 8D 05 xx xx xx xx
    //         movzx r8d, byte ptr [rsp+xx]      ; 44 0F B6 44 24 xx
    // 如何找到: 上述函数内 "v5 = &unk_XXX + 64 * a3" 这行对应的汇编
    //   unk_XXX 就是数组基址. 每个"条目"64字节 = 2个RawWeaponDataField(每个32字节)
    //   所以实际的RawWeaponDataField数量 = weaponSettingsArrSize * 2
    std::uint64_t weaponSettingsArr = Pattern::FindPattern<std::uint64_t>(ctx.data, ("48 8D 05 ? ? ? ? 44 0F B6 44 24 ?"), 7);

    // [SIG] weaponSettingsArrSize - 数组边界检查里的立即数
    // 反汇编: lea eax, [r8-1]           ; 41 8D 40 FF     (a3 - 1)
    //         cmp eax, 0x4F7            ; 3D F7 04 00 00  (与数组上限比较)
    //         ja error                  ; 0F 87 xx xx xx xx
    // 如何找到: 上述函数开头的范围检查 `(unsigned int)(a3 - 1) > 0x4F7`
    //   0x4F7 后面跟的就是报错 "Invalid weapon setting index #%d."
    // proc逻辑: addr+5 指向 cmp 的 imm32, 直接读出来就是数组大小上限
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
