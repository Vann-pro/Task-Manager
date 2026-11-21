#!/usr/bin/env bash
# Mengompilasi C++ -> WebAssembly dengan Emscripten. Hasil: web/wasm/ptm.js (WASM tertanam, SINGLE_FILE).
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v emcc >/dev/null 2>&1; then
  echo "emcc tidak ditemukan. Pasang Emscripten (https://emscripten.org/docs/getting_started/downloads.html)" >&2
  echo "lalu jalankan: source /path/ke/emsdk/emsdk_env.sh" >&2
  exit 1
fi
mkdir -p web/wasm
emcc src/task.cpp src/priority.cpp src/json.cpp src/task_manager.cpp src/wasm_api.cpp \
  -std=c++17 -O2 -Wall \
  -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME=createPTMModule -s ENVIRONMENT=web \
  -s ALLOW_MEMORY_GROWTH=1 -s SINGLE_FILE=1 \
  -s "EXPORTED_FUNCTIONS=['_ptm_load','_ptm_add','_ptm_update','_ptm_delete','_ptm_get','_ptm_query','_ptm_stats','_ptm_state','_ptm_export','_ptm_import']" \
  -s "EXPORTED_RUNTIME_METHODS=['cwrap']" \
  -o web/wasm/ptm.js
echo "Selesai: web/wasm/ptm.js"
