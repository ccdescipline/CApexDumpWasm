# DumpCore static library (platform-independent)
add_library(DumpCore STATIC
    core/DumpCore.cpp
    core/Pattern.cpp
    core/Convar.cpp
    core/dataTable.cpp
    core/buttons.cpp
    core/dataMap.cpp
    core/Mics.cpp
    core/weaponSettings.cpp
    3rd/PS.cpp
)

target_include_directories(DumpCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # for "NT/NTHeader.h", "3rd/PS.h" etc.
    ${CMAKE_CURRENT_SOURCE_DIR}/core     # for "dumpContext.h", "Pattern.h" etc.
)

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found via vcpkg, fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()
target_link_libraries(DumpCore PUBLIC nlohmann_json::nlohmann_json)

# Platform-specific targets
if(EMSCRIPTEN)
    add_subdirectory(wasm)
else()
    add_subdirectory(native)
endif()
