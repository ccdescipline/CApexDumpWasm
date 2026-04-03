#pragma once
#include "dumpContext.h"
#include <map>
#include <string>
#include <cstdint>

class Convar {
public:
    static bool dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output);
};
