#!/usr/bin/env bash
# Compiles the firmware effect engine to WebAssembly for the browser simulator.
# Needs Emscripten (brew install emscripten, or emsdk). Output: simulator/build/crabfx.js
set -euo pipefail
cd "$(dirname "$0")"
LIB=../firmware/lib/crabfx/src
mkdir -p build
em++ -std=c++17 -O2 -Wall \
  -I"$LIB" "$LIB"/*.cpp wasm_api.cpp \
  -sMODULARIZE=1 -sEXPORT_NAME=createCrabFx -sSINGLE_FILE=1 -sENVIRONMENT=web \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_RUNTIME_METHODS=cwrap,UTF8ToString,HEAPU8,HEAPF32 \
  -o build/crabfx.js
echo "built simulator/build/crabfx.js"
