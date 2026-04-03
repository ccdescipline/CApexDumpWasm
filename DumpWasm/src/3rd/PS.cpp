#include "PS.h"
#include "NT/NTHeader.h"

auto PS::Search(const char* data, size_t size, const char* sigs, const char* mask) -> uint64_t
{
    auto dataStart = data;
    auto dataEnd = dataStart + size;
    auto sigsLength = std::strlen(mask);

    for (auto i = dataStart; i < dataEnd; i++)
    {
        if (i[0] != sigs[0] && mask[0] != '?')
            continue;

        auto done = true;
        for (auto x = size_t(0); x < sigsLength; x++)
        {
            if (mask[x] == '?')
                continue;

            if (sigs[x] != i[x])
            {
                done = false;
                break;
            }
        }
        if (!done) continue;

        return i - dataStart;
    }

    return 0;
}

auto PS::SearchMultiple(const char* data, size_t size, const char* sigs, const char* mask) -> std::vector<uint64_t>
{
    auto dataStart = data;
    auto dataEnd = dataStart + size;
    auto sigsLength = std::strlen(mask);

    auto list = std::vector<uint64_t>();
    for (auto i = dataStart; i < dataEnd; i++)
    {
        if (i[0] != sigs[0] && mask[0] != '?')
            continue;

        auto done = true;
        for (auto x = size_t(0); x < sigsLength; x++)
        {
            if (mask[x] == '?')
                continue;

            if (i[x] != sigs[x])
            {
                done = false;
                break;
            }
        }
        if (!done) continue;

        list.emplace_back(i - dataStart);
    }

    return list;
}

auto PS::SearchInSection(const char* data, const char* name, const char* sigs, const char* mask) -> uint64_t
{
    auto DH = reinterpret_cast<const IMAGE_DOS_HEADER*>(data);
    if (DH->e_magic != IMAGE_DOS_SIGNATURE) return 0;

    auto NT = reinterpret_cast<const IMAGE_NT_HEADERS*>(data + DH->e_lfanew);
    if (NT->Signature != IMAGE_NT_SIGNATURE) return 0;

    auto SH = IMAGE_FIRST_SECTION(NT);
    for (auto i = 0; i < NT->FileHeader.NumberOfSections; i++)
    {
        char cstr[9];
        cstr[sizeof(cstr) - 1] = '\0';
        std::memcpy(cstr, SH[i].Name, sizeof(cstr) - 1);
        if (std::strcmp(cstr, name)) continue;

        auto offset = Search(data + SH[i].VirtualAddress, SH[i].Misc.VirtualSize, sigs, mask);
        if (!offset) return 0;

        return SH[i].VirtualAddress + offset;
    }

    return 0;
}

auto PS::SearchInSectionMultiple(const char* data, const char* name, const char* sigs, const char* mask) -> std::vector<uint64_t>
{
    auto list = std::vector<uint64_t>();
    auto DH = reinterpret_cast<const IMAGE_DOS_HEADER*>(data);
    if (DH->e_magic != IMAGE_DOS_SIGNATURE) return list;

    auto NT = reinterpret_cast<const IMAGE_NT_HEADERS*>(data + DH->e_lfanew);
    if (NT->Signature != IMAGE_NT_SIGNATURE) return list;

    auto SH = IMAGE_FIRST_SECTION(NT);
    for (auto i = 0; i < NT->FileHeader.NumberOfSections; i++)
    {
        char cstr[9];
        cstr[sizeof(cstr) - 1] = '\0';
        std::memcpy(cstr, SH[i].Name, sizeof(cstr) - 1);
        if (std::strcmp(cstr, name)) continue;

        auto buffer = SearchMultiple(data + SH[i].VirtualAddress, SH[i].Misc.VirtualSize, sigs, mask);
        if (!buffer.size()) return list;

        for (auto offset : buffer) list.emplace_back(SH[i].VirtualAddress + offset);
        break;
    }

    return list;
}

auto PS::In(uint64_t data, size_t size, uint64_t addr, size_t addr_size) -> bool
{
    return (uint64_t)data <= addr && (uint64_t)data <= addr + addr_size && addr + addr_size < (uint64_t)data + size;
}

auto PS::isAsciiOnly(const char* str) -> bool {
    if (!str) return false;
    while (*str) {
        unsigned char c = static_cast<unsigned char>(*str);
        if (c < 33 || c > 126) return false;
        ++str;
    }
    return true;
}
