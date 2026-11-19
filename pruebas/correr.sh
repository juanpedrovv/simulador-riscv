#!/usr/bin/env bash
# Corre cada programa en el simulador y revisa que la salida tenga el resultado esperado.
# Uso: ./pruebas/correr.sh   (o make test)
cd "$(dirname "$0")/.."
SIM=./build/rvsim
ok=0; total=0

probar() {   # probar nombre archivo "comandos" "texto esperado"
    total=$((total + 1))
    salida=$(printf "$3" | $SIM "$2" 2>&1)
    if grep -qF -- "$4" <<< "$salida"; then
        echo "OK     $1"; ok=$((ok + 1))
    else
        echo "FALLA  $1 (se esperaba: $4)"
    fi
}

probar "quicksort"           programas/quicksort.bin       'run\nmem 0x1000 0x101B\nexit\n' \
       '0x00001000: 01 00 00 00 02 00 00 00 03 00 00 00 04 00 00 00'
probar "quicksort (fin)"     programas/quicksort.bin       'run\nmem 0x1010 0x101B\nexit\n' \
       'Memoria (0x00001010-0x0000101B): 06 00 00 00 08 00 00 00 09 00 00 00'
probar "arbol simetrico"     programas/arbol_simetrico.bin 'run\nregs a0\nexit\n'  'x10 (a0  ) = 0x00000001'
probar "riscvtest"           programas/riscvtest.bin       'run\nmemw 100\nexit\n' 'Mem[0x00000064] = 0x00000019  (25)'
probar "proyecto 2"          programas/proyecto2.bin       'run\nmemw 0x624\nexit\n' 'Mem[0x00000624] = 0x0000004A  (74)'
probar "suma con ecall"      programas/suma_ecall.bin      'run\n10\nexit\n'   '1 + 2 + ... + n = 55'
probar "carga de ELF"        programas/quicksort.elf       'run\nmemw 0x1018\nexit\n' 'Mem[0x00001018] = 0x00000009  (9)'
probar "todas las instr."    pruebas/todas_instrucciones.bin 'run\nregs a0\nexit\n' 'x10 (a0  ) = 0x00000001'
probar "instruccion ilegal"  /dev/null                     'step\nexit\n'      'instrucción ilegal 0x00000000'

echo "$ok de $total pruebas correctas"
[ "$ok" -eq "$total" ]
