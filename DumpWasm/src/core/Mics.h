#pragma once
#include "dumpContext.h"
#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Mics {
public:
    static bool dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output, std::vector<std::string>& errorlist);
};
