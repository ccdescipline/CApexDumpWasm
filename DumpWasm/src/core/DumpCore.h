#pragma once
#include <cstdint>
#include <string>

namespace DumpCore {

struct DumpResult {
    std::string json;
    std::string errors;
    bool success;
};

DumpResult dumpAll(const uint8_t* data, size_t size);

} // namespace DumpCore
