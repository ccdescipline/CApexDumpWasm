#include "DumpCore.h"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dump_file> [output.json]" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    file.close();

    std::cout << "File loaded: " << data.size() << " bytes" << std::endl;

    auto result = DumpCore::dumpAll(data.data(), data.size());

    if (!result.success) {
        std::cerr << "Dump failed." << std::endl;
        return 1;
    }

    if (result.errors != "[]") {
        std::cout << "Warnings: " << result.errors << std::endl;
    }

    const char* outPath = (argc >= 3) ? argv[2] : "result.json";
    std::ofstream out(outPath);
    if (!out) {
        std::cerr << "Failed to open output file: " << outPath << std::endl;
        return 1;
    }
    out << result.json;
    out.close();

    std::cout << "Done. Output written to: " << outPath << std::endl;

    return 0;
}
