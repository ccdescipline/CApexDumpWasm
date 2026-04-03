#pragma once
#include <cstdint>
#include <string>
#include <utility>

class dumpContext {
public:
    std::string data;
    std::uint64_t baseAddress;

    dumpContext(std::string d, std::uint64_t base)
        : data(std::move(d)), baseAddress(base) {}
};
