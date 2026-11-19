# Lee n con ecall, calcula 1 + 2 + ... + n y lo imprime.
# Usa las environment calls de SPIM: a7 = 4 print string, 5 read int, 1 print int, 10 exit.
.globl _start
_start:
    la   a0, pedir
    li   a7, 4              # print string
    ecall
    li   a7, 5              # read int -> a0
    ecall
    mv   s0, a0             # s0 = n

    li   s1, 0              # s1 = suma
    li   t0, 1              # t0 = i
bucle:
    blt  s0, t0, mostrar    # si n < i termina
    add  s1, s1, t0
    addi t0, t0, 1
    j    bucle

mostrar:
    la   a0, resultado
    li   a7, 4
    ecall
    mv   a0, s1
    li   a7, 1              # print int
    ecall
    li   a0, '\n'
    li   a7, 11             # print char
    ecall
    li   a7, 10             # exit
    ecall

.section .rodata
pedir:     .string "Ingrese n: "
resultado: .string "1 + 2 + ... + n = "
