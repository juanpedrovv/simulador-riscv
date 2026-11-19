# Simulador RISC-V (RV32I)

Tarea 5 de Arquitectura de Computadores (CS3051), UTEC 2026-2.

Simulador por consola del RV32I. Carga un programa ya compilado (binario crudo o ELF) y lo
ejecuta paso a paso, mostrando el PC, los registros y la memoria.

## Compilar y correr

```
make
./build/rvsim programas/quicksort.bin
make test
```

## Comandos

```
step [n]        ejecuta n instrucciones
run             ejecuta hasta el final o hasta un breakpoint
pc              muestra el PC
regs [r ...]    muestra los registros (x5, a0, sp...)
mem ini [fin]   muestra la memoria en bytes
memw dir [n]    muestra n palabras desde dir
dis [dir] [n]   desensambla
break dir       pone un breakpoint (delete dir lo quita)
reset           vuelve a cargar el programa
exit            sale
```

## Carpetas

- `src`: código del simulador
- `programas`: programas de prueba en `.s`, `.bin` y `.elf`. `ensamblar.sh` los vuelve a generar con `riscv64-unknown-elf`.
- `pruebas`: prueba de las 37 instrucciones y el script que usa `make test`
- `informe`: informe de la tarea
