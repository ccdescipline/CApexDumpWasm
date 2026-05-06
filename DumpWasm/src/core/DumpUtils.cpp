#include "DumpUtils.h"

#include <cstring>

namespace DumpUtils {

uint32_t ReadU32(const std::string& data, uint64_t offset) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());
    return static_cast<uint32_t>(bytes[offset]) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

int32_t ReadI32(const std::string& data, uint64_t offset) {
    const auto value = ReadU32(data, offset);
    int32_t signedValue;
    std::memcpy(&signedValue, &value, sizeof(signedValue));
    return signedValue;
}

} // namespace DumpUtils
