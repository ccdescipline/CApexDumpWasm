# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A web-based tool that parses Apex Legends PE dump files and extracts game offsets. Two main parts:

- **DumpWasm/** — C++ dump logic, builds as WASM (production) or native EXE (testing)
- **WASMLoader/** — Vue 3 + Vite frontend that loads the WASM module

## Build Commands

### Frontend (WASMLoader)
```bash
cd WASMLoader
npm install
npm run dev       # Dev server at http://localhost:5174
npm run build     # Production build to dist/
```

### C++ (DumpWasm) — four presets, CLion can switch via toolbar

| Preset | Command | Output | LogE |
|--------|---------|--------|------|
| `wasm-release` | `cmake --preset wasm-release && cmake --build build/wasm-release` | `WASMLoader/src/wasm/` | off |
| `wasm-debug` | `cmake --preset wasm-debug && cmake --build build/wasm-debug` | `build/wasm-debug/` | on |
| `native-debug` | `cmake --preset native-debug && cmake --build build/native-debug` | `build/native-debug/DumpNative.exe` | on |
| `native-release` | `cmake --preset native-release && cmake --build build/native-release` | `build/native-release/DumpNative.exe` | off |

Native EXE usage: `DumpNative.exe <dump_file>` — reads file, prints JSON to stdout.

Requires: Emscripten SDK at `C:/emsdk`, vcpkg at `C:/vcpkg` with `nlohmann-json` and `xorstr` installed.

## Architecture

### Directory Structure (DumpWasm/src/)
```
src/
├── core/           # Platform-independent dump logic (static library: DumpCore)
│   ├── DumpCore.h/cpp    — top-level dumpAll() entry point
│   ├── dumpContext.h      — shared context (PE data + ImageBase)
│   ├── Pattern.h/cpp      — signature scanning with RVA resolution
│   ├── Convar.h/cpp       — console variable offsets
│   ├── dataTable.h/cpp    — RecvTable/RecvProp (3 pattern variants)
│   ├── buttons.h/cpp      — console command offsets
│   ├── dataMap.h/cpp      — entity datamap structures
│   ├── Mics.h/cpp         — misc offsets (LocalPlayer, EntityList, ViewMatrix, etc.)
│   └── weaponSettings.h/cpp — weapon config offsets
├── 3rd/
│   ├── PS.h/cpp   — low-level byte-pattern search (Search, SearchInSection, etc.)
│   └── Log.h      — LogE macro (enabled by DUMP_DEBUG, set automatically in Debug builds)
├── NT/
│   └── NTHeader.h — all PE format structs (DOS, NT, Section headers + type aliases)
├── wasm/
│   └── wasm_main.cpp — emscripten exports (_dumpAll, _add), links DumpCore
└── native/
    └── main.cpp       — reads file from disk, calls DumpCore, prints JSON
```

### Build Design
- **DumpCore** is a static library containing all dump logic — no platform dependencies
- **wasm/** and **native/** are thin entry points that link against DumpCore
- CMake uses `if(EMSCRIPTEN)` to choose which entry point to build
- `LogE` macro is controlled by `DUMP_DEBUG` compile definition, auto-set in Debug builds

### Data Flow (browser)
1. User uploads PE dump → `loadWasm.vue` → Web Worker (`wasmWorker.js`)
2. Worker allocates WASM heap, copies buffer, calls `_dumpAll(ptr, size, outPtr, errPtr)`
3. C++ `DumpCore::dumpAll()` validates PE headers, creates `dumpContext`, runs all 6 dump modules
4. Returns JSON string via `char**` out-params → Worker reads with `UTF8ToString()` → UI displays

### Pattern Scanning
- `PS::Search` / `PS::SearchInSection` — raw byte+mask pattern matching against PE sections
- `Pattern::FindPattern` — higher-level: scans full binary, optional RVA resolution
- `Pattern::FindPatternByProc` — scans + applies custom lambda for offset extraction

## Adding a New Dump Module
1. Create `src/core/MyModule.h` (class declaration) + `src/core/MyModule.cpp` (implementation)
2. Add `core/MyModule.cpp` to the `DumpCore` source list in `src/CMakeLists.txt`
3. Include and call it in `DumpCore.cpp`

## Dependencies
- **C++ (vcpkg):** nlohmann-json, xorstr
- **C++ (toolchain):** Emscripten (WASM builds only)
- **JS:** Vue 3, Element Plus, @wasmer/wasi, Vite
