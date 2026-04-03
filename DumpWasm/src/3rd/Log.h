#ifndef DUMPWASM_LOG_H
#define DUMPWASM_LOG_H

#include <cstdarg>
#include <cstdio>
#include <iostream>

class debug {
public:
    static void log(const char* str, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, str);
        vsnprintf(buffer, sizeof(buffer), str, args);
        va_end(args);
        std::cout << buffer << std::endl;
    }
};

#ifdef DUMP_DEBUG
    #define LogE(str, ...) debug::log((str), ##__VA_ARGS__)
#else
    #define LogE(str, ...)
#endif

#endif // DUMPWASM_LOG_H
