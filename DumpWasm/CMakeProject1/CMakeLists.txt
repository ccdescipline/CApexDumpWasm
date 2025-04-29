# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。
add_executable (DumpWasm "CMakeProject1.cpp" "CMakeProject1.h" "NT/NTHeader.h" "dump/dumpContext.h" "dump/Convar.h" "dump/Pattern.h" "3rd/PS.h" "3rd/PS.cpp"
        dump/dataTable.h
        dump/buttons.h
        dump/dataMap.h
        dump/Mics.h
        3rd/Log.h)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DumpWasm PROPERTY CXX_STANDARD 20)
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../WASMLoader/src/wasm)

target_link_options(DumpWasm PRIVATE
  "SHELL:-sEXPORT_ES6=1"
  "SHELL:-sMODULARIZE=1"
  "SHELL:-sEXPORTED_FUNCTIONS=['_malloc','_free','_dumpAll','UTF8ToString']"
  "SHELL:-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8','HEAP32']"
  "SHELL:-sALLOW_MEMORY_GROWTH=1"
  "SHELL:-sEXIT_RUNTIME=0"
  "SHELL:-sASYNCIFY"


  
)

# TODO: 如有需要，请添加测试并安装目标。
