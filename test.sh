#!/usr/bin/env bash
# Menjalankan uji logika C++ secara native (butuh g++ atau clang++).
set -euo pipefail
cd "$(dirname "$0")/.."
CXX="${CXX:-g++}"
$CXX -std=c++17 -Wall -Wextra -g src/*.cpp tests/test_core.cpp -o /tmp/ptm_test
/tmp/ptm_test
