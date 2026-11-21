@echo off
rem Uji logika C++ secara native. Butuh g++ (MinGW/MSYS2) di PATH.
cd /d "%~dp0\.."
g++ -std=c++17 -Wall -Wextra -g src/task.cpp src/priority.cpp src/json.cpp src/task_manager.cpp src/wasm_api.cpp tests/test_core.cpp -o ptm_test.exe
if errorlevel 1 exit /b 1
ptm_test.exe
