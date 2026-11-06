#!/usr/bin/env bash
# Сборка симулятора собственной ISA (режимы 1-3).
set -euo pipefail
cd "$(dirname "$0")"

LLVM_CONFIG="${LLVM_CONFIG:-llvm-config}"
CXX="${CXX:-clang++}"
CC="${CC:-clang}"
LLVM_FLAGS="$("$LLVM_CONFIG" --cxxflags --ldflags --libs --system-libs)"

$CC -c -I include app/sim_stub.c -o sim_stub.o
# shellcheck disable=SC2086
$CXX -std=c++17 -I . main.cpp bin.cpp cpu.cpp instr.cpp extIR.cpp fullIR.cpp \
     sim_stub.o $LLVM_FLAGS -o asmsim
rm -f sim_stub.o

echo "built: ./asmsim  (LLVM $("$LLVM_CONFIG" --version))"
