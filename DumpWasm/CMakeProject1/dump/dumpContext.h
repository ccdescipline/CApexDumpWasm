#pragma once
#include <cstdint>
#include <string>

class dumpContext {
	public:
	std::string data;
	std::uint64_t baseAddress;

	dumpContext(std::string _d, std::uint64_t _base) : data(_d), baseAddress(_base){

	}
};