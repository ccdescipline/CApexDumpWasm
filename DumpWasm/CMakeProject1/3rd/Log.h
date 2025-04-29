//
// Created by admin on 2025/4/29.
//

#ifndef CMAKEPROJECT1_LOG_H
#define CMAKEPROJECT1_LOG_H
#include <cstdarg>
#include <cstdio>
#include <iostream>



class debug{
    public:
        static void log(const char* str,...){
            // 预分配一个初始缓冲区
            char buffer[1024];

            // 获取变长参数列表
            va_list args;
            va_start(args, str);

            // 使用 vsnprintf 格式化到 buffer
            int needed = vsnprintf(buffer, sizeof(buffer), str, args);
            va_end(args);

            std::cout << buffer << std::endl;
        }
};

//#define LogE(str, ...) debug::log((str), __VA_ARGS__);
#define LogE(str, ...)

#endif //CMAKEPROJECT1_LOG_H
