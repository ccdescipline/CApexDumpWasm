add_executable(DumpWasm wasm_main.cpp)
target_link_libraries(DumpWasm PRIVATE DumpCore)

# Release builds output directly to WASMLoader
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(DumpWasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../WASMLoader/src/wasm"
    )
endif()

target_link_options(DumpWasm PRIVATE
    "SHELL:-sEXPORT_ES6=1"
    "SHELL:-sMODULARIZE=1"
    "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dumpAll','UTF8ToString']"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8','HEAP32']"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sEXIT_RUNTIME=0"
    "SHELL:-sASYNCIFY"
)
