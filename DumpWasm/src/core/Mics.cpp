#include "Mics.h"
#include "Pattern.h"
#include "3rd/PS.h"
#include "3rd/Log.h"
#include "NT/NTHeader.h"
#include <iostream>

bool Mics::dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errors) {
    uint64_t LocalPlayer = Pattern::FindPattern<uint64_t>(ctx.data, ("48 8D 0D ? ? ? ? 48 8B D7 FF 50 58"), 7);
    LogE("LocalPlayer : 0x%llx", LocalPlayer);
    if (!LocalPlayer) {
        errors.push_back("LocalPlayer not found");
    } else {
        LocalPlayer += 8;
        output["LocalPlayer"] = LocalPlayer;
    }

    uint64_t EntityList = (uint64_t)Pattern::FindPattern(ctx.data, ("E8 ? ? ? ? 48 8D 1D ? ? ? ? 83 F8 01"), 12);
    LogE("EntityList : 0x%llx", EntityList);
    if (!EntityList) {
        errors.push_back("EntityList not found");
    } else {
        output["EntityList"] = EntityList;
    }

    uint64_t ViewRender = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 01 FF 50 70 48 8D 15"), 7);
    LogE("ViewRender : 0x%llx", ViewRender);
    if (!ViewRender) {
        errors.push_back("ViewRender not found");
    } else {
        output["ViewRender"] = ViewRender;
    }

    uint64_t ViewMatrix = (uint64_t)Pattern::FindPatternByProc(ctx.data, ("0F 11 9F ? ? ? ? 48 89 3D ? ? ? ?"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 17));
    });
    LogE("ViewMatrix : 0x%llx", ViewMatrix);
    if (!ViewMatrix) {
        errors.push_back("ViewMatrix not found");
    } else {
        output["ViewMatrix"] = ViewMatrix;
    }

    uint64_t NameList = (uint64_t)Pattern::FindPattern(ctx.data, ("48 63 43 38 48 8D 0D ? ? ? ? 48 8D 04 40 48 8B 44 C1 E8"), 11);
    LogE("NameList : 0x%llx", NameList);
    if (!NameList) {
        errors.push_back("NameList not found");
    } else {
        output["NameList"] = NameList;
    }

    uint64_t GlobalVars = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 15 ? ? ? ? FF 50 10 85 C0"), 7);
    LogE("GlobalVars : 0x%llx", GlobalVars);
    if (!GlobalVars) {
        errors.push_back("GlobalVars not found");
    } else {
        output["GlobalVars"] = GlobalVars;
    }

    uint64_t NetworkVarTablePtr = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D ? ? ? ? ? 83 7C ? ? ? 74 ? 48 8B D7 48 8D 0D ? ? ? ? E8 ? ? ? ? EB"), 7);
    LogE("NetworkVarTablePtr : 0x%llx", NetworkVarTablePtr);
    if (!NetworkVarTablePtr) {
        errors.push_back("NetworkVarTablePtr not found");
    } else {
        output["NetworkVarTablePtr"] = NetworkVarTablePtr;
    }

    uint64_t InputSystem = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 0D ? ? ? ? 85 FD 74 ? 41 3B DE"), 7);
    LogE("InputSystem : 0x%llx", InputSystem);
    if (!InputSystem) {
        errors.push_back("InputSystem not found");
    } else {
        output["InputSystem"] = InputSystem;
    }

    uint64_t LevelName = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8D 15 ? ? ? ? 48 8B 45 F8 48 8D 0D ? ? ? ?"), 7);
    LogE("LevelName : 0x%llx", LevelName);
    if (!LevelName) {
        errors.push_back("LevelName not found");
    } else {
        output["LevelName"] = LevelName;
    }

    uint64_t highlightSetting = (uint64_t)Pattern::FindPattern(ctx.data, ("48 8B 05 ? ? ? ? 8B 54 01 30"), 7);
    LogE("highlightSetting : 0x%llx", highlightSetting);
    if (!highlightSetting) {
        errors.push_back("highlightSetting not found");
    } else {
        output["highlightSetting"] = highlightSetting;
    }

    uint64_t WeaponSettingsMeta_base = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("4C 8B 83 ? ? ? ? 4C 8D 8B"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 10));
    });
    LogE("WeaponSettingsMeta_base : 0x%llx", WeaponSettingsMeta_base);
    if (!WeaponSettingsMeta_base) {
        errors.push_back("WeaponSettingsMeta_base not found");
    } else {
        output["WeaponSettingsMeta_base"] = WeaponSettingsMeta_base;
    }

    uint64_t studioHdr = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("48 8B 9F ? ? ? ? 48 85 DB 75 0C"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("studioHdr : 0x%llx", studioHdr);
    if (!studioHdr) {
        errors.push_back("studioHdr not found");
    } else {
        output["studioHdr"] = studioHdr;
    }

    uint64_t lastVisibleTime = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("8B 8B ? ? ? ? 89 08 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 48 85 C0 74 08 8B 8B ? ? ? ? 89 08 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 48 85 C0 74 08 8B 8B ? ? ? ? 89 08 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 48 85 C0 74 08"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 2));
    });
    if (!lastVisibleTime) {
        errors.push_back("lastVisibleTime not found");
    } else {
        output["lastVisibleTime"] = lastVisibleTime;
    }

    uint64_t m_vecAbsOrigin = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("F3 0F 7E 97"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 4));
    });
    LogE("m_vecAbsOrigin : 0x%llx", m_vecAbsOrigin);
    if (!m_vecAbsOrigin) {
        errors.push_back("m_vecAbsOrigin not found");
    } else {
        output["m_vecAbsOrigin"] = m_vecAbsOrigin;
    }

    uint64_t camera_origin = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("0F 2E 89 ? ? ? ? 7A 62"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("camera_origin : 0x%llx", camera_origin);
    if (!camera_origin) {
        errors.push_back("camera_origin not found");
    } else {
        output["camera_origin"] = camera_origin;
    }

    uint64_t commandNumber = (uintptr_t)(Pattern::FindPattern(ctx.data, ("45 33 F6 8B 2D ? ? ? ?"), 9));
    LogE("commandNumber : 0x%llx", commandNumber);
    if (!commandNumber) {
        errors.push_back("commandNumber not found");
    } else {
        output["commandNumber"] = commandNumber;
    }

    uintptr_t cinput = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 8D 0D ? ? ? ? 33 D2 48 83 C4 20"), 7));
    LogE("cinput : 0x%llx", cinput);
    if (!cinput) {
        errors.push_back("cinput not found");
    } else {
        output["cinput"] = cinput;
    }

    uintptr_t CHLClient = (Pattern::FindPattern<uintptr_t>(ctx.data, ("48 8B 0D ? ? ? ? 48 8B 74 24 50"), 7));
    LogE("CHLClient :%llx", CHLClient);
    if (!CHLClient) {
        errors.push_back("CHLClient not found");
    } else {
        output["CHLClient"] = CHLClient;
    }

    uintptr_t observerList = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF ? ? 48 85 C0 74 ? 48 63 4E 38"), 7));
    LogE("observerList : 0x%llx", observerList);
    if (!observerList) {
        errors.push_back("observerList not found");
    } else {
        output["observerList"] = observerList;
    }

    uint64_t observer_index = Pattern::FindPatternByProc<uint64_t>(ctx.data, ("8B 84 C8 ? ? ? ? 83 F8"), [&](uint64_t addr, uint64_t base) -> uint64_t {
        return (uint64_t)(*(UINT32*)((uint64_t)addr + 3));
    });
    LogE("observer_index : 0x%llx", observer_index);
    if (!observer_index) {
        errors.push_back("observer_index not found");
    } else {
        output["observer_index"] = observer_index;
    }

    uintptr_t netChannel = (uintptr_t)(Pattern::FindPattern(ctx.data, ("48 ? ? ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 40 0F B6 F7"), 7));
    LogE("netChannel : %llx", netChannel);
    if (netChannel == NULL) {
        errors.push_back("netChannel not found");
    } else {
        output["netChannel"] = netChannel;
    }

    uintptr_t ClientState = Pattern::FindPattern<uintptr_t>(ctx.data, ("E8 ? ? ? ? 48 81 C4 68 04 00 00 C3"), 5);
    LogE("ClientState : %llx", ClientState);
    if (ClientState == NULL) {
        errors.push_back("ClientState not found");
    } else {
        output["ClientState"] = ClientState;
    }

    uintptr_t SignonState = Pattern::FindPattern<uintptr_t>(ctx.data, ("C6 41 14 01 80 3D ? ? ? ? ? 74 68 80 3D ? ? ? ? ? 75 09 83 3D"), 28);
    LogE("SignonState : %llx", SignonState);
    if (SignonState == NULL) {
        errors.push_back("SignonState not found");
    } else {
        SignonState += 1;
        output["SignonState"] = SignonState;
    }

    uintptr_t localplayerHandle = (uintptr_t)(Pattern::FindPattern(ctx.data, ("44 0F 29 48 ? 8B 05 ? ? ? ? 83 F8 FF"), 11));
    LogE("localplayerHandle : %llx", localplayerHandle);
    if (localplayerHandle == NULL) {
        errors.push_back("localplayerHandle not found");
    } else {
        output["localplayerHandle"] = localplayerHandle;
    }

    uintptr_t WeaponNames = Pattern::FindPattern<uintptr_t>(ctx.data, ("74 12 48 8B 0D ? ? ? ? 48 8B 01 FF 50 58"), 9);
    LogE("WeaponNames : 0x%p", WeaponNames);
    if (WeaponNames == NULL) {
        errors.push_back("WeaponNames not found");
    } else {
        output["WeaponNames"] = WeaponNames;
    }

    uintptr_t ModelNames = Pattern::FindPattern<uintptr_t>(ctx.data, ("48 8B 0D ? ? ? ? 48 85 C9 74 0F 8D 42 01"), 7);
    LogE("ModelNames : 0x%p", ModelNames);
    if (ModelNames == NULL) {
        errors.push_back("ModelNames not found");
    } else {
        output["ModelNames"] = ModelNames;
    }

    return true;
}
