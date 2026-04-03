# CApexDumpWasm

A web-based tool for parsing **Apex Legends** PE dump files and extracting game offsets, built with **Vue 3** and **WebAssembly**.

> Live tool: [https://github.com/ccdescipline/CApexDumpWasm](https://github.com/ccdescipline/CApexDumpWasm)

## Features

- Upload a PE dump file directly in the browser — no server required
- C++ dump logic compiled to WASM runs entirely client-side
- Extracts and displays:
  - **Mics** — LocalPlayer, EntityList, ViewMatrix, and other misc offsets
  - **RecvTable** — networked entity properties
  - **DataMap** — entity datamap structures
  - **Buttons** — console command offsets
  - **Convars** — console variable values
  - **WeaponSettings** — weapon configuration offsets
  - **ModelNames / WeaponNames** — model and weapon name tables
- Search results with keyword highlighting, section context labels, and result count
- Click a search result to jump to its position in the output
- Copy output to clipboard with one click
- Export full results as a structured JSON file
- Version number displayed in the UI (sourced from the WASM module)

## Built With

- [Vue 3](https://vuejs.org/) + [Element Plus](https://element-plus.org/)
- [Vite](https://vitejs.dev/)
- C++ compiled to [WebAssembly](https://webassembly.org/) via [Emscripten](https://emscripten.org/)
- [nlohmann/json](https://github.com/nlohmann/json)

## Usage

1. Open the app in your browser.
2. Click **Select File** and upload a PE dump file from Apex Legends.
3. Wait for parsing to complete (runs in a Web Worker).
4. Browse the output, use **Search** to filter offsets, or **Export to Json** to download.

## Build

### Frontend

```bash
cd WASMLoader
npm install
npm run dev      # dev server at http://localhost:5174
npm run build    # production build to dist/
```

### C++ / WASM

Requires: Emscripten SDK at `C:/emsdk`, vcpkg at `C:/vcpkg` with `nlohmann-json` and `xorstr`.

| Preset | Output |
|--------|--------|
| `wasm-release` | `WASMLoader/src/wasm/` (production) |
| `wasm-debug` | `build/wasm-debug/` |
| `native-debug` | `build/native-debug/DumpNative.exe` |
| `native-release` | `build/native-release/DumpNative.exe` |

```bash
cmake --preset wasm-release && cmake --build build/wasm-release
```

Native EXE usage: `DumpNative.exe <dump_file>` — prints JSON to stdout.

## Notes

This tool is intended for research and educational purposes only.

---

Contributions and issue reports are welcome.
