#pragma once
#include <cstdint>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using BYTE = std::uint8_t;
using ULONGLONG = std::uint64_t;
using PBYTE = std::uint8_t*;
using UINT32 = std::uint32_t;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    std::uint16_t   e_magic;                     // Magic number
    std::uint16_t   e_cblp;                      // Bytes on last page of file
    std::uint16_t   e_cp;                        // Pages in file
    std::uint16_t   e_crlc;                      // Relocations
    std::uint16_t   e_cparhdr;                   // Size of header in paragraphs
    std::uint16_t   e_minalloc;                  // Minimum extra paragraphs needed
    std::uint16_t   e_maxalloc;                  // Maximum extra paragraphs needed
    std::uint16_t   e_ss;                        // Initial (relative) SS value
    std::uint16_t   e_sp;                        // Initial SP value
    std::uint16_t   e_csum;                      // Checksum
    std::uint16_t   e_ip;                        // Initial IP value
    std::uint16_t   e_cs;                        // Initial (relative) CS value
    std::uint16_t   e_lfarlc;                    // File address of relocation table
    std::uint16_t   e_ovno;                      // Overlay number
    std::uint16_t   e_res[4];                    // Reserved words
    std::uint16_t   e_oemid;                     // OEM identifier (for e_oeminfo)
    std::uint16_t   e_oeminfo;                   // OEM information; e_oemid specific
    std::uint16_t   e_res2[10];                  // Reserved words
    std::uint16_t   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    WORD        MajorOperatingSystemVersion;
    WORD        MinorOperatingSystemVersion;
    WORD        MajorImageVersion;
    WORD        MinorImageVersion;
    WORD        MajorSubsystemVersion;
    WORD        MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        Subsystem;
    WORD        DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    DWORD       LoaderFlags;
    DWORD       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, * PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    std::uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, * PIMAGE_NT_HEADERS64;