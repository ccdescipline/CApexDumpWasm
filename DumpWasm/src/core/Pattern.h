#pragma once
#include <functional>
#include <cstdint>
#include <string>
#include "NT/NTHeader.h"

#define RVA(Instr, InstrSize) ((std::uint64_t)Instr + InstrSize + *(long*)((std::uint64_t)Instr + (InstrSize - sizeof(long))))

class Pattern {
public:
    static PBYTE FindPattern_Wrapper(PBYTE start, size_t size, const char* pattern);

    template<typename Ret = PBYTE>
    static Ret FindPattern(const std::string& dumpdata, const char* pattern, int RVAsize = 0) {
        PBYTE findaddress = FindPattern_Wrapper((PBYTE)dumpdata.data(), dumpdata.length(), pattern);

        if (!findaddress) {
            return (Ret)0;
        }

        if (RVAsize > 0) {
            findaddress = (PBYTE)RVA(findaddress, RVAsize);
        }

        return (Ret)(((std::uint64_t)findaddress - (std::uint64_t)dumpdata.data()));
    }

    template<typename Ret = PBYTE>
    static Ret FindPatternByProc(const std::string& dumpdata, const char* pattern, std::function<uint64_t(uint64_t, uint64_t)> proc) {
        PBYTE findaddress = FindPattern_Wrapper((PBYTE)dumpdata.data(), dumpdata.length(), pattern);

        if (!findaddress) {
            return (Ret)0;
        }

        return (Ret)(proc(reinterpret_cast<uint64_t>(findaddress), reinterpret_cast<uint64_t>(dumpdata.data())));
    }
};
