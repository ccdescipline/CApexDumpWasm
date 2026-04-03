cmake_minimum_required(VERSION 3.16)

# vcpkg toolchain for native builds (presets override this for emscripten)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(DumpWasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug builds enable logging via LogE macro
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DUMP_DEBUG)
endif()

add_subdirectory(src)
