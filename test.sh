#!/bin/bash

set -eu

cd $(dirname "$0")

./build.sh

tests=(
    "01-special.gb"
    "02-interrupts.gb"
    "03-op sp,hl.gb"
    "04-op r,imm.gb"
    "05-op rp.gb"
    "06-ld r,r.gb"
    "07-jr,jp,call,ret,rst.gb"
    "08-misc instrs.gb"
    "09-op r,r.gb"
    "10-bit ops.gb"
    "11-op a,(hl).gb"
)
for test in "${tests[@]}"; do
    ./build/main "gb-test-roms/cpu_instrs/individual/${test}" | sed '/^$/d' || true
    python3 -c "print('-'*40)"
done