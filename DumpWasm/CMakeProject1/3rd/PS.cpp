#include "PS.h"

typedef struct _IMAGE_DOS_HEADER { // DOS .EXE header
	uint16_t e_magic;              // Magic number
	uint16_t e_cblp;               // Bytes on last page of file
	uint16_t e_cp;                 // Pages in file
	uint16_t e_crlc;               // Relocations
	uint16_t e_cparhdr;            // Size of header in paragraphs
	uint16_t e_minalloc;           // Minimum extra paragraphs needed
	uint16_t e_maxalloc;           // Maximum extra paragraphs needed
	uint16_t e_ss;                 // Initial (relative) SS value
	uint16_t e_sp;                 // Initial SP value
	uint16_t e_csum;               // Checksum
	uint16_t e_ip;                 // Initial IP value
	uint16_t e_cs;                 // Initial (relative) CS value
	uint16_t e_lfarlc;             // File address of relocation table
	uint16_t e_ovno;               // Overlay number
	uint16_t e_res[4];             // Reserved words
	uint16_t e_oemid;              // OEM identifier (for e_oeminfo)
	uint16_t e_oeminfo;            // OEM information; e_oemid specific
	uint16_t e_res2[10];           // Reserved words
	int32_t  e_lfanew;             // File address of new exe header
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
} IMAGE_FILE_HEADER, * PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
	uint32_t VirtualAddress;
	uint32_t Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER {
	uint16_t             Magic;
	uint8_t              MajorLinkerVersion;
	uint8_t              MinorLinkerVersion;
	uint32_t             SizeOfCode;
	uint32_t             SizeOfInitializedData;
	uint32_t             SizeOfUninitializedData;
	uint32_t             AddressOfEntryPoint;
	uint32_t             BaseOfCode;
	uint64_t             ImageBase;
	uint32_t             SectionAlignment;
	uint32_t             FileAlignment;
	uint16_t             MajorOperatingSystemVersion;
	uint16_t             MinorOperatingSystemVersion;
	uint16_t             MajorImageVersion;
	uint16_t             MinorImageVersion;
	uint16_t             MajorSubsystemVersion;
	uint16_t             MinorSubsystemVersion;
	uint32_t             Win32VersionValue;
	uint32_t             SizeOfImage;
	uint32_t             SizeOfHeaders;
	uint32_t             CheckSum;
	uint16_t             Subsystem;
	uint16_t             DllCharacteristics;
	uint64_t             SizeOfStackReserve;
	uint64_t             SizeOfStackCommit;
	uint64_t             SizeOfHeapReserve;
	uint64_t             SizeOfHeapCommit;
	uint32_t             LoaderFlags;
	uint32_t             NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER, * PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
	uint32_t              Signature;
	IMAGE_FILE_HEADER     FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, * PIMAGE_NT_HEADERS;

typedef struct _IMAGE_SECTION_HEADER {
	uint8_t Name[8];
	union {
		uint32_t PhysicalAddress;
		uint32_t VirtualSize;
	} Misc;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
} IMAGE_SECTION_HEADER, * PIMAGE_SECTION_HEADER;

#define IMAGE_FIRST_SECTION(NT) reinterpret_cast<const IMAGE_SECTION_HEADER*>(reinterpret_cast<uint64_t>(NT) + __builtin_offsetof(IMAGE_NT_HEADERS, OptionalHeader) + NT->FileHeader.SizeOfOptionalHeader)

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
	if (DH->e_magic != 0x5A4D) return 0;

	auto NT = reinterpret_cast<const IMAGE_NT_HEADERS*>(data + DH->e_lfanew);
	if (NT->Signature != 0x4550) return 0;

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
	if (DH->e_magic != 0x5A4D) return list;

	auto NT = reinterpret_cast<const IMAGE_NT_HEADERS*>(data + DH->e_lfanew);
	if (NT->Signature != 0x4550) return list;

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
//	return (
//		addr > reinterpret_cast<uint64_t>(data) &&
//		((addr + addr_size) < (reinterpret_cast<uint64_t>(data) + size)) &&
//		((addr + addr_size) > (reinterpret_cast<uint64_t>(data)))
//		);

    return (uint64_t)data <= addr && (uint64_t)data <= addr + addr_size && addr + addr_size < (uint64_t)data + size;
}

auto PS::isAsciiOnly(const char* str) -> bool {
    if (!str) return false; // null 指针直接返回 false
    while (*str) {
        unsigned char c = static_cast<unsigned char>(*str);
        if (c < 33 || c > 126) return false; // 33~126 才是可见 ASCII
        ++str;

    }
    return true;
}


