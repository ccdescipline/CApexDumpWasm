// CMakeProject1.cpp: 定义应用程序的入口点。
//
#include <iostream>
#include "emscripten.h"
#include "CMakeProject1.h"
#include "NT\NTHeader.h"
#include <string>
#include "dump\Convar.h"
//#include "include\json.hpp"
#include <nlohmann/json.hpp>
#include "dump/dataTable.h"
#include "dump/buttons.h"
#include "dump/dataMap.h"
#include "dump/Mics.h"
#include "dump/weaponSettings.h"

auto  get_image_base_from_pe_string(const std::string& pe_data) ->std::uint64_t {
	// 1. 安全检查
	if (pe_data.size() < sizeof(IMAGE_DOS_HEADER)) {
		throw std::runtime_error("PE data too small for DOS header");
	}

	// 2. 获取 DOS Header
	const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(pe_data.data());
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		throw std::runtime_error("Invalid DOS signature");
	}

	// 3. 安全检查 NT header 偏移是否在范围内
	if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > pe_data.size()) {
		throw std::runtime_error("NT header out of range");
	}

	// 4. 获取 NT Headers
	const IMAGE_NT_HEADERS64* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pe_data.data() + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) {
		throw std::runtime_error("Invalid NT signature");
	}

	// 5. 返回 ImageBase（64位或32位都可以扩展判断）
	return nt->OptionalHeader.ImageBase;
}

static std::string printstr = "hello world";
static std::string printstrError = "hello world";

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    int add(int a, int b) {
        return a + b;
    }

	EMSCRIPTEN_KEEPALIVE
    bool dumpAll(const uint8_t* data, size_t size,char ** output,char ** outputError) {
		// 将数据转换为 std::vector<uint8_t> 或 std::string 来处理
		std::string dataStr(reinterpret_cast<const char*>(data), size);

        try {
            auto base =  get_image_base_from_pe_string(dataStr);
            std::cout << "base is 0x" << std::hex << base << std::dec << std::endl;
            //printstr = std::format("base is {:#016x}", base);
            dumpContext ctx = dumpContext(dataStr, base);

            nlohmann::json resJson;
            nlohmann::json errorJson;
            std::vector<std::string> errorList;

            std::map<std::string, uint64_t> convars;
            Convar::dump(ctx, convars);
            resJson["Convars"] = convars;

            std::map<std::string, std::map<std::string, uint64_t>> dataTable;
            dataTable::dump(ctx, dataTable);
            resJson["RecvTable"] = dataTable;

            std::map<std::string, uint64_t> buttons;
            buttons::dump(ctx,buttons);
            resJson["Buttons"] = buttons;

            std::map<std::string, std::map<std::string, uint64_t>> dataMap;
            dataMap::dump(ctx, dataMap);
            resJson["dataMap"] = dataMap;

            std::map<std::string, uint64_t> misc;
            Mics::dump(ctx,misc,errorList);
            resJson["Mics"] = misc;

            std::map<std::string, uint64_t> weaponsettings;
            weaponSettings::dump(ctx, weaponsettings);
            resJson["weaponSettings"] = weaponsettings;

            std::cout << "start dump" << std::endl;
            printstr = resJson.dump();
            *output = printstr.data();
            std::cout << "end dump" << std::endl;

            errorJson = errorList;
            printstrError = errorJson.dump();
            *outputError = printstrError.data();
        }catch (const std::exception& e) {
            std::cout << "Exception caught: " << e.what() << std::endl;
            return false;
        }


        return true;
	}


}

int main()
{
	std::cout << "Hello CMake." << std::endl;
	return 0;
}
