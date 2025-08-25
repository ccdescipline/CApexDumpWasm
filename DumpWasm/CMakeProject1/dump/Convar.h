#pragma once
#include "dumpContext.h"
#include <map>
#include "Pattern.h"
#include <iostream>
#include "../3rd/PS.h"
#include "../include/json.hpp"
#include "../3rd/Log.h"

class Convar {
public:
	inline static bool dump(dumpContext ctx,std::map<std::string, uint64_t>& output) {
		struct RawConVar
		{
			std::uint64_t m_vTable;          // 0x0000
			std::uint64_t m_pNext;           // 0x0008
//			std::uint64_t m_bRegistered;     // 0x0010
			std::uint64_t m_pszName;         // 0x0018
			std::uint64_t m_pszDescription;  // 0x0020
			std::uint64_t m_pszDataType;     // 0x0028
			std::uint32_t m_iFlags;          // 0x0030
			char     pad_0034[4];       // 0x0034
			std::uint64_t m_pParent;         // 0x0038
			std::uint64_t m_pTable;          // 0x0040
			std::uint64_t m_pszDefaultValue; // 0x0048
			std::uint64_t m_pszString;       // 0x0050
			std::uint64_t m_iStringLength;   // 0x0058
			float    m_fValue;          // 0x0060
			std::int32_t  m_nValue;          // 0x0064
			bool     m_bHasMin;         // 0x0068
			bool     m_bHasMax;         // 0x0069
			char     pad_006A[2];       // 0x006A
			float    m_fMinValue;       // 0x006C
			float    m_fMaxValue;       // 0x0070
		};

		auto offsets = std::map<std::string, uint64_t>();

		std::uint64_t conVarVtable = Pattern::FindPattern<std::uint64_t>(ctx.data,("48 8B 79 ? 48 8D 05 ? ? ? ? 48 89 ? 48 8D"), 11);

		if (!conVarVtable) {
            std::cout << "can't find conVarVtable" << std::endl;
			return false;
		}

        LogE("conVarVtable %p",conVarVtable);

		//output += std::format("conVarVtable {:#016x} \r\n", conVarVtable);

		conVarVtable = reinterpret_cast<std::uint64_t>( conVarVtable + ctx.baseAddress);

		auto convarbyte = [](std::uint64_t a1) {
			std::vector<uint8_t> res;
			for (int i = 0; i < sizeof(std::uint64_t); ++i) {
				uint8_t byte = (a1 >> (i * 8)) & 0xFF;
				//std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
				res.push_back(byte);
			}
			return res;
		};

		auto matches = PS::SearchInSectionMultiple(ctx.data.c_str(), ".data", (const char*)convarbyte(conVarVtable).data(), "xxxxxxxx");

		//output += std::format("matches {} \r\n", matches.size());

		if (!matches.size()){
            std::cout << "matches Null " << std::endl;
            return false;
        }

        LogE("conVarVtable matches %d",matches.size());


		for (auto i = size_t(); i < matches.size(); i++) {
			auto offset = matches[i];

			RawConVar* convar = (RawConVar*)(ctx.data.data() + offset);

            if(!PS::In(ctx.baseAddress,ctx.data.size(),convar->m_pszName,8)){
                continue;
            }

			if (!convar->m_pszDescription || !convar->m_pszName) {
				continue;
			}


			std::string name = std::string((char*)convar->m_pszName - ctx.baseAddress + (std::uint64_t)ctx.data.data());

            if(!PS::isAsciiOnly(name.data())) continue;

			offsets[name] = offset;



			//output += std::format("name {} value {:#x} \r\n", name.data(), (offset));
            LogE("name %s : 0x%llx", name.data(),offset);
		}


        output = offsets;

        return true;
	}
};