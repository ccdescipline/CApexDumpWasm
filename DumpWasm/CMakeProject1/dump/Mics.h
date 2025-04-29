//
// Created by admin on 2025/4/29.
//

#ifndef CMAKEPROJECT1_MICS_H
#define CMAKEPROJECT1_MICS_H

#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include <unwind.h>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../NT/NTHeader.h"
#include "../3rd/Log.h"

class Mics{
public:
    inline static bool dump(dumpContext ctx,std::map<std::string, uint64_t>& output){
        uint64_t LocalPlayer = Pattern::FindPattern<uint64_t>(ctx.data,("48 8D 0D ? ? ? ? 48 8B D7 FF 50 58"),7) + 8 ;
        LogE("LocalPlayer : 0x%llx", LocalPlayer);
        if (!LocalPlayer) {
            return false;
        }
        output["LocalPlayer"] = LocalPlayer;


        uint64_t EntityList = (uint64_t)Pattern::FindPattern(ctx.data,("E8 ? ? ? ? 48 8D 1D ? ? ? ? 83 F8 01"), 12) ;
        LogE("EntityList : 0x%llx", EntityList);
        if (!EntityList) {
            return false;
        }
        output["EntityList"] = EntityList;

        uint64_t ViewRender = (uint64_t)Pattern::FindPattern(ctx.data,("48 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 01 FF 50 70 48 8D 15"),7) ;
        LogE("ViewRender : 0x%llx", ViewRender);
        if (!ViewRender) {
            return false;
        }
        output["ViewRender"] = ViewRender;

        uint64_t NameList = (uint64_t)Pattern::FindPattern(ctx.data,("48 63 43 38 48 8D 0D ? ? ? ? 48 8D 04 40 48 8B 44 C1 E8"), 11) ;
        LogE("NameList : 0x%llx", NameList);
        if (!NameList) {
            return false;
        }
        output["NameList"] = NameList;

        uint64_t GlobalVars = (uint64_t)Pattern::FindPattern(ctx.data,("48 8D 15 ? ? ? ? FF 50 10 85 C0"), 7);
        LogE("GlobalVars : 0x%llx", GlobalVars);
        if (!GlobalVars) {
            return false;
        }
        output["GlobalVars"] = GlobalVars;

        uint64_t NetworkVarTablePtr = (uint64_t)Pattern::FindPattern(ctx.data,("48 8D 15 ? ? ? ? 83 7C 11 0C 07 74 ? 48 8B D7"), 7) ;
        LogE("NetworkVarTablePtr : 0x%llx", NetworkVarTablePtr);
        if (!NetworkVarTablePtr) {
            return false;
        }
        output["NetworkVarTablePtr"] = NetworkVarTablePtr;

        uint64_t InputSystem = (uint64_t)Pattern::FindPattern(ctx.data,("48 8D 0D ? ? ? ? 85 FD 74 ? 41 3B DE"), 7) ;
        LogE("InputSystem : 0x%llx", InputSystem);
        if (!InputSystem) {
            return false;
        }
        output["InputSystem"] = InputSystem;

        uint64_t LevelName = (uint64_t)Pattern::FindPattern(ctx.data,("48 8D 15 ? ? ? ? 48 8B 45 F8 48 8D 0D ? ? ? ?"), 7) ;
        LogE("LevelName : 0x%llx", LevelName);
        if (!LevelName) {
            return false;
        }
        output["LevelName"] = LevelName;
//Highlight_SetParam
//48 8B 15 ? ? ? ?  4C 8D 04 76
        uint64_t highlightSetting = (uint64_t)Pattern::FindPattern(ctx.data,("48 8B 05 ? ? ? ? 8B 54 01 30"), 7) ;
        LogE("highlightSetting : 0x%llx", highlightSetting);
        if (!highlightSetting) {
            return false;
        }
        output["highlightSetting"] = highlightSetting;

//WeaponSettingsMeta_base = *(UINT32*)((FindPattern(E("4C 8D 8B ? ? ? ? 4C 8B C6")) + 3));

        uint64_t WeaponSettingsMeta_base = Pattern::FindPatternByProc<uint64_t>(ctx.data,("4C 8B 83 ? ? ? ? 4C 8D 8B"), [&](uint64_t addr, uint64_t base)->uint64_t {

            return (uint64_t)(*(UINT32*)((uint64_t)addr + 10));
        });
        LogE("WeaponSettingsMeta_base : 0x%llx", WeaponSettingsMeta_base);
        if (!WeaponSettingsMeta_base) {
            return false;
        }
        output["WeaponSettingsMeta_base"] = WeaponSettingsMeta_base;

///studioHdr = *(UINT32*)(FindPattern(E("48 8B 9F ? ? ? ? 48 85 DB 75 0C")) + 3);

        uint64_t studioHdr = Pattern::FindPatternByProc<uint64_t>(ctx.data,("48 8B 9F ? ? ? ? 48 85 DB 75 0C"), [&](uint64_t addr, uint64_t base)->uint64_t {
            return (uint64_t)(*(UINT32*)( (uint64_t)addr + 3));
        });
        LogE("studioHdr : 0x%llx", studioHdr);
        if (!studioHdr) {
            return false;
        }
        output["studioHdr"] = studioHdr;

        //find string lastVisibleTime
        uint64_t lastVisibleTime = Pattern::FindPatternByProc<uint64_t>(ctx.data,("F3 41 0F 10 81 ? ? ? ? 66 3B 77 4E 0F 83 ? ? ? ? 4C 8B 57 30 45 0F B7 62 ? 66 45 85 E4 0F 84 ? ? ? ?"),[&](uint64_t addr, uint64_t base)->uint64_t {
            return (uint64_t)(*(UINT32*)( (uint64_t)addr + 5));
        });

        output["lastVisibleTime"] = lastVisibleTime;

//m_vecAbsOrigin = *(UINT32*)((FindPattern(E("F3 0F 7E 97")) + 4));

        uint64_t m_vecAbsOrigin = Pattern::FindPatternByProc<uint64_t>(ctx.data,("F3 0F 7E 97"), [&](uint64_t addr, uint64_t base)->uint64_t {
            return (uint64_t)(*(UINT32*)(  (uint64_t)addr + 4));
        });
        LogE("m_vecAbsOrigin : 0x%llx", m_vecAbsOrigin);
        if (!m_vecAbsOrigin) {
            return false;
        }
        output["m_vecAbsOrigin"] = m_vecAbsOrigin;

//camera_origin = *(UINT32*)((FindPattern(E("0F 2E 89 ? ? ? ? 7A 58")) + 3));
        uint64_t camera_origin = Pattern::FindPatternByProc<uint64_t>(ctx.data,("0F 2E 89 ? ? ? ? 7A 58"), [&](uint64_t addr, uint64_t base)->uint64_t {
            return (uint64_t)(*(UINT32*)( (uint64_t)addr + 3));
        });
        LogE("camera_origin : 0x%llx", camera_origin);
        if (!camera_origin) {
            return false;
        }
        output["camera_origin"] = camera_origin;

        uint64_t commandNumber = (uintptr_t)(Pattern::FindPattern(ctx.data,("45 33 F6 8B 2D ? ? ? ?"), 9));
        LogE("commandNumber : 0x%llx", commandNumber);
        if (!commandNumber) {
            return false;
        }
        output["commandNumber"] = commandNumber;

        uintptr_t cinput = (uintptr_t)(Pattern::FindPattern(ctx.data,("48 8D 0D ? ? ? ? 33 D2 48 83 C4 20"), 7)) ;
        LogE("cinput : 0x%llx", cinput);
        if (!cinput) {
            return false;
        }
        output["cinput"] = cinput;

//-buildcubemaps
        uintptr_t CHLClient = (Pattern::FindPattern<uintptr_t>(ctx.data,("48 8B 0D ? ? ? ? 48 8B 74 24 58"), 7));

        LogE("CHLClient :%llx", CHLClient);
        if (!CHLClient) {
            return false;
        }
        output["CHLClient"] = CHLClient;


        uintptr_t observerList = (uintptr_t)(Pattern::FindPattern(ctx.data,("48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF ? ? 48 85 C0 74 ? 48 63 4E 38"), 7));
        LogE("observerList : 0x%llx", observerList);
        if (!observerList) {
            return false;
        }
        output["observerList"] = observerList;


        uintptr_t netChannel = (uintptr_t)(Pattern::FindPattern(ctx.data,("48 ? ? ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 0F B6 F3"), 7));
        LogE("netChannel : %llx", netChannel);

        if (netChannel == NULL) {
            return false;
        }
        output["netChannel"] = netChannel;

        uintptr_t ClientState = Pattern::FindPattern<uintptr_t>(ctx.data,("E8 ? ? ? ? 48 81 C4 68 04 00 00 C3"), 5);
        LogE("ClientState : %llx", ClientState);
        if (ClientState == NULL) {
            return false;
        }
        output["ClientState"] = ClientState;

        uintptr_t SignonState = Pattern::FindPattern<uintptr_t>(ctx.data,("C6 41 14 01 80 3D ? ? ? ? ? 74 68 80 3D ? ? ? ? ? 75 09 83 3D"), 28) + 1;
        LogE("SignonState : %llx", SignonState);
        if (SignonState == NULL) {
            return false;
        }
        output["SignonState"] = SignonState;

//44 0F 29 48 ? 8B 05 ? ? ? ? 83 F8 FF
        uintptr_t localplayerHandle = (uintptr_t)(Pattern::FindPattern(ctx.data,("44 0F 29 48 ? 8B 05 ? ? ? ? 83 F8 FF"), 11));

        LogE("localplayerHandle : %llx", localplayerHandle);

        if (localplayerHandle == NULL) {
            return false;
        }
        output["localplayerHandle"] = localplayerHandle;

        return true;
    }
};

#endif //CMAKEPROJECT1_MICS_H
