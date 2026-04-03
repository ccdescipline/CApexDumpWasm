#include "Pattern.h"

PBYTE Pattern::FindPattern_Wrapper(PBYTE start, size_t size, const char* Pattern)
{
#define InRange(x, a, b) (x >= a && x <= b)
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

    PBYTE ModuleStart = start;
    PBYTE ModuleEnd = (PBYTE)(ModuleStart + size);

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

#undef InRange
#undef GetBits
#undef GetByte
}
