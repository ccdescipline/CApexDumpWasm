#include "DumpCore.h"
#include "dumpContext.h"
#include "Convar.h"
#include "dataTable.h"
#include "buttons.h"
#include "dataMap.h"
#include "Mics.h"
#include "weaponSettings.h"
#include "NT/NTHeader.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

static uint64_t get_image_base_from_pe_string(const std::string& pe_data) {
    if (pe_data.size() < sizeof(IMAGE_DOS_HEADER)) {
        throw std::runtime_error("PE data too small for DOS header");
    }

    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(pe_data.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        throw std::runtime_error("Invalid DOS signature");
    }

    if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > pe_data.size()) {
        throw std::runtime_error("NT header out of range");
    }

    const IMAGE_NT_HEADERS64* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pe_data.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        throw std::runtime_error("Invalid NT signature");
    }

    return nt->OptionalHeader.ImageBase;
}

namespace DumpCore {

DumpResult dumpAll(const uint8_t* data, size_t size) {
    DumpResult result;
    result.success = false;

    std::string dataStr(reinterpret_cast<const char*>(data), size);

    try {
        auto base = get_image_base_from_pe_string(dataStr);
        std::cout << "base is 0x" << std::hex << base << std::dec << std::endl;

        dumpContext ctx(std::move(dataStr), base);

        nlohmann::json resJson;
        std::vector<std::string> errorList;

        std::map<std::string, uint64_t> convars;
        Convar::dump(ctx, convars);
        resJson["Convars"] = convars;

        std::map<std::string, std::map<std::string, uint64_t>> dataTableResult;
        dataTable::dump(ctx, dataTableResult);
        resJson["RecvTable"] = dataTableResult;

        std::map<std::string, uint64_t> buttonsResult;
        buttons::dump(ctx, buttonsResult);
        resJson["Buttons"] = buttonsResult;

        std::map<std::string, std::map<std::string, uint64_t>> dataMapResult;
        dataMap::dump(ctx, dataMapResult);
        resJson["dataMap"] = dataMapResult;

        std::map<std::string, uint64_t> misc;
        Mics::dump(ctx, misc, errorList);
        resJson["Mics"] = misc;

        std::map<std::string, uint64_t> weaponsettings;
        weaponSettings::dump(ctx, weaponsettings);
        resJson["weaponSettings"] = weaponsettings;

        resJson["version"] = VERSION;
        result.json = resJson.dump(4);

        nlohmann::json errorJson = errorList;
        result.errors = errorJson.dump();
        result.success = true;
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }

    return result;
}

} // namespace DumpCore
