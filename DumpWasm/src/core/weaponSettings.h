#pragma once
#include "dumpContext.h"
#include <map>
#include <string>
#include <cstdint>

class weaponSettings {
public:
    static bool dump(const dumpContext& ctx, std::map<std::string, uint64_t>& output);

private:
    static void readEntity(const dumpContext& ctx, std::map<std::string, uint64_t>& items, uint64_t addr);
};
