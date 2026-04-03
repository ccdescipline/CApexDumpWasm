#pragma once
#include "dumpContext.h"
#include <map>
#include <string>
#include <cstdint>

class dataTable {
public:
    static bool dump(const dumpContext& ctx, std::map<std::string, std::map<std::string, uint64_t>>& output);
};
