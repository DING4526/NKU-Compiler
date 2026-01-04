#!/bin/bash

INPUT_FILE="${1:-test.s}"
OUTPUT_BIN="${2:-test.bin}"
OBJ_FILE="${INPUT_FILE%.s}.o"

if [ -f "toolchains.conf" ]; then
    source toolchains.conf
fi

RISCV_GCC="${RISCV_GCC:-riscv64-linux-gnu-gcc}"
TEXT_ADDR="${TEXT_ADDR:-}"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found"
    exit 1
fi

"$RISCV_GCC" "$INPUT_FILE" -c -o "$OBJ_FILE" -w -g -Wa,--gdwarf-5

# 当 TEXT_ADDR 为空时，不使用 -Ttext 参数
if [ -n "$TEXT_ADDR" ]; then
    "$RISCV_GCC" "$OBJ_FILE" -o "$OUTPUT_BIN"\
        -L./lib -lsysy_riscv\
        -static -mcmodel=medany\
        -Wl,--no-relax,-Ttext="$TEXT_ADDR"
else
    "$RISCV_GCC" "$OBJ_FILE" -o "$OUTPUT_BIN"\
        -L./lib -lsysy_riscv\
        -static -mcmodel=medany\
        -Wl,--no-relax
fi

rm "$OBJ_FILE"

echo "Successfully compiled $INPUT_FILE to $OUTPUT_BIN"
