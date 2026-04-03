#include "DumpCore.h"
#include <emscripten.h>
#include <iostream>
#include <string>

static std::string g_output;
static std::string g_errorOutput;

extern "C" {

EMSCRIPTEN_KEEPALIVE
int add(int a, int b) {
    return a + b;
}

EMSCRIPTEN_KEEPALIVE
bool dumpAll(const uint8_t* data, size_t size, char** output, char** outputError) {
    auto result = DumpCore::dumpAll(data, size);

    g_output = std::move(result.json);
    g_errorOutput = std::move(result.errors);

    *output = g_output.data();
    *outputError = g_errorOutput.data();

    return result.success;
}

}

int main() {
    std::cout << "WASM module loaded." << std::endl;
    return 0;
}
