#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <vector>

namespace PS
{
	auto Search(const char* data, size_t size, const char* sigs, const char* mask) -> uint64_t;
	auto SearchMultiple(const char* data, size_t size, const char* sigs, const char* mask) -> std::vector<uint64_t>;

	auto SearchInSection(const char* data, const char* name, const char* sigs, const char* mask) -> uint64_t;
	auto SearchInSectionMultiple(const char* data, const char* name, const char* sigs, const char* mask) -> std::vector<uint64_t>;

	auto In(uint64_t data, size_t size, uint64_t addr, size_t addr_size) -> bool;

    auto isAsciiOnly(const char* str) -> bool;
}
