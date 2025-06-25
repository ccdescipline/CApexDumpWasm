#pragma once
#include <functional>
#include <cstdint>
#include <string>
#include "../NT/NTHeader.h"
#include <cstddef> // �����ǹٷ���׼�����λ��

#define RVA(Instr, InstrSize) ((std::uint64_t)Instr + InstrSize + *(long*)((std::uint64_t)Instr + (InstrSize - sizeof(long))))


class Pattern {
public:

	

	//// ��ȡģ�鳤��
	//inline static std::uint32_t GetModuleLen(std::uint64_t hModule)
	//{
	//	// //MUTATE

	//	PBYTE pImage = (PBYTE)hModule;
	//	PIMAGE_DOS_HEADER pImageDosHeader;
	//	PIMAGE_NT_HEADERS pImageNtHeader;
	//	pImageDosHeader = (PIMAGE_DOS_HEADER)pImage;
	//	if (pImageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	//	{
	//		return 0;
	//	}
	//	pImageNtHeader = (PIMAGE_NT_HEADERS)&pImage[pImageDosHeader->e_lfanew];
	//	if (pImageNtHeader->Signature != IMAGE_NT_SIGNATURE)
	//	{
	//		return 0;
	//	}
	//	return pImageNtHeader->OptionalHeader.SizeOfImage;

	//	////MUTATE_END
	//}

	inline static PBYTE FindPattern_Wrapper(PBYTE start, size_t  size, const char* Pattern)
	{
		//MUTATE

		//find pattern utils
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

//get module range
		PBYTE ModuleStart = start;
		PBYTE ModuleEnd = (PBYTE)(ModuleStart + size);

		//scan pattern main
		PBYTE FirstMatch = nullptr;
		const char* CurPatt = Pattern;
		for (; ModuleStart < ModuleEnd; ++ModuleStart)
		{
			bool SkipByte = (*CurPatt == '\?');
			if (SkipByte || *ModuleStart == GetByte(CurPatt)) {
				if (!FirstMatch) FirstMatch = ModuleStart;
				SkipByte ? CurPatt += 2 : CurPatt += 3;
				if (CurPatt[-1] == 0) return FirstMatch;
			}

			else if (FirstMatch) {
				ModuleStart = FirstMatch;
				FirstMatch = nullptr;
				CurPatt = Pattern;
			}
		}

		return NULL;

		//MUTATE_END
	}

	template<typename Ret = PBYTE>
	inline static Ret FindPattern(std::string dumpdata,const char* Pattern, int RVAsize = 0) {



		PBYTE findaddress = FindPattern_Wrapper((PBYTE)dumpdata.data(), dumpdata.length(), Pattern);

        if(!findaddress){
            return NULL;
        }

		if (RVAsize > 0) {
			findaddress = (PBYTE)RVA(findaddress, RVAsize);
		}

		return  (Ret)(((std::uint64_t)findaddress - (std::uint64_t)dumpdata.data()));
	}

	template<typename Ret = PBYTE>
	inline static Ret FindPatternByProc(std::string dumpdata, const char* Pattern, std::function<uint64_t (uint64_t, uint64_t)> proc) {


		PBYTE findaddress = FindPattern_Wrapper((PBYTE)dumpdata.data(), dumpdata.length(), Pattern);

        if(!findaddress){
            return NULL;
        }

		return reinterpret_cast<Ret>(proc(reinterpret_cast<uint64_t>(findaddress), reinterpret_cast<uint64_t>(dumpdata.data()))) ;
	}
};