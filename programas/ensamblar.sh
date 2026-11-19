#!/usr/bin/env bash
# Genera el binario crudo (.bin, cargado desde 0x00000000, igual que el "Raw" de CPUlator)
# y el ejecutable (.elf) de cada .s de la carpeta. Necesita el toolchain riscv64-unknown-elf.
# Uso: ./ensamblar.sh [carpeta]   (por defecto la carpeta del script)
set -euo pipefail
cd "${1:-$(dirname "$0")}"
tmp=$(mktemp -d)
for src in *.s; do
    n=${src%.s}
    riscv64-unknown-elf-as -march=rv32i -mabi=ilp32 -mno-relax "$src" -o "$tmp/$n.o"
    riscv64-unknown-elf-ld -m elf32lriscv -Ttext=0 -e 0 --no-relax "$tmp/$n.o" -o "$n.elf"
    riscv64-unknown-elf-objcopy -O binary "$n.elf" "$n.bin"
    echo "$n: $(stat -c %s "$n.bin") bytes"
done
rm -rf "$tmp"
