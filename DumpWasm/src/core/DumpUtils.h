#pragma once

#include <cstdint>
#include <string>

namespace DumpUtils {

uint32_t ReadU32(const std::string& data, uint64_t offset);
int32_t ReadI32(const std::string& data, uint64_t offset);

} // namespace DumpUtils
