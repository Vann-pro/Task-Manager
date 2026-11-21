@echo off
rem Mengompilasi C++ -> WebAssembly dengan Emscripten. Hasil: web\wasm\ptm.js (WASM tertanam, SINGLE_FILE).
rem Jalankan dari "Command Prompt" setelah mengaktifkan emsdk (emsdk_env.bat).
cd /d "%~dp0\.."
where emcc >nul 2>nul
if errorlevel 1 (
  echo emcc tidak ditemukan. Pasang Emscripten lalu jalankan emsdk_env.bat di jendela ini.
  exit /b 1
)
if not exist web\wasm mkdir web\wasm
call emcc src/task.cpp src/priority.cpp src/json.cpp src/task_manager.cpp src/wasm_api.cpp ^
  -std=c++17 -O2 -Wall ^
  -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME=createPTMModule -s ENVIRONMENT=web ^
  -s ALLOW_MEMORY_GROWTH=1 -s SINGLE_FILE=1 ^
  -s "EXPORTED_FUNCTIONS=['_ptm_load','_ptm_add','_ptm_update','_ptm_delete','_ptm_get','_ptm_query','_ptm_stats','_ptm_state','_ptm_export','_ptm_import']" ^
  -s "EXPORTED_RUNTIME_METHODS=['cwrap']" ^
  -o web/wasm/ptm.js
if errorlevel 1 exit /b 1
echo Selesai: web\wasm\ptm.js
