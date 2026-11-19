# Prueba las 37 instrucciones del anexo A.
# Cada prueba compara un registro con el valor esperado; si no coincide salta a falla.
# Al final: a0 = 1 si todo salio bien. Si algo falla: a0 = 0 y a1 = numero de la prueba.

.macro CHECK reg, valor, n
    li   s11, \n
    li   t6, \valor
    bne  \reg, t6, falla
.endm

.globl _start
_start:
    # --- lui, addi, auipc ---
    lui  t0, 0x12345
    CHECK t0, 0x12345000, 1
    addi t0, t0, 0x678
    CHECK t0, 0x12345678, 2
    addi t0, zero, -1
    CHECK t0, 0xFFFFFFFF, 3
    jal  t1, aqui               # t1 = direccion de "aqui"
aqui:
    auipc t2, 0                 # t2 = pc = direccion de "aqui"
    bne  t1, t2, falla4
    auipc t2, 1                 # t2 = pc + 0x1000
    sub  t2, t2, t1
    CHECK t2, 0x1008, 5

    # --- tipo R ---
    li   a2, -7                 # 0xFFFFFFF9
    li   a3, 3
    add  t0, a2, a3
    CHECK t0, -4, 6
    sub  t0, a3, a2
    CHECK t0, 10, 7
    and  t0, a2, a3
    CHECK t0, 1, 8
    or   t0, a2, a3
    CHECK t0, -5, 9
    xor  t0, a2, a3
    CHECK t0, -6, 10
    slt  t0, a2, a3             # -7 < 3 con signo
    CHECK t0, 1, 11
    sltu t0, a2, a3             # 0xFFFFFFF9 < 3 sin signo es falso
    CHECK t0, 0, 12
    sll  t0, a3, a3
    CHECK t0, 24, 13
    srl  t0, a2, a3
    CHECK t0, 0x1FFFFFFF, 14
    sra  t0, a2, a3
    CHECK t0, -1, 15
    li   a4, 33                 # solo cuentan los 5 bits de abajo: shift de 1
    sll  t0, a3, a4
    CHECK t0, 6, 16

    # --- tipo I de la ALU ---
    slti t0, a2, -6
    CHECK t0, 1, 17
    sltiu t0, a3, -1            # -1 es 0xFFFFFFFF sin signo
    CHECK t0, 1, 18
    xori t0, a3, -1             # not
    CHECK t0, -4, 19
    ori  t0, a3, 0x70
    CHECK t0, 0x73, 20
    andi t0, a2, 0xF
    CHECK t0, 9, 21
    slli t0, a3, 30
    CHECK t0, 0xC0000000, 22
    srli t0, a2, 28
    CHECK t0, 0xF, 23
    srai t0, a2, 1
    CHECK t0, -4, 24

    # --- stores y loads (little-endian) ---
    li   s0, 0x2000
    li   t0, 0x80FF7F01
    sw   t0, 0(s0)              # bytes: 01 7F FF 80
    lb   t1, 0(s0)
    CHECK t1, 1, 25
    lb   t1, 2(s0)
    CHECK t1, -1, 26
    lbu  t1, 2(s0)
    CHECK t1, 0xFF, 27
    lh   t1, 2(s0)
    CHECK t1, 0xFFFF80FF, 28
    lhu  t1, 2(s0)
    CHECK t1, 0x80FF, 29
    lh   t1, 0(s0)
    CHECK t1, 0x7F01, 30
    li   t0, 0x1234
    sb   t0, 4(s0)              # solo guarda 0x34
    li   t0, 0xABCD
    sh   t0, 6(s0)
    lw   t1, 4(s0)              # bytes: 34 00 CD AB
    CHECK t1, 0xABCD0034, 31
    sw   t0, -4(s0)             # offset negativo
    lw   t1, -4(s0)
    CHECK t1, 0xABCD, 32

    # --- branches: a2 = -7, a3 = 3 ---
    li   s11, 33
    beq  a2, a3, falla
    beq  a3, a3, 1f
    j    falla
1:  li   s11, 34
    bne  a3, a3, falla
    bne  a2, a3, 1f
    j    falla
1:  li   s11, 35
    blt  a3, a2, falla          # 3 < -7 es falso
    blt  a2, a3, 1f
    j    falla
1:  li   s11, 36
    bge  a2, a3, falla
    bge  a3, a3, 1f
    j    falla
1:  li   s11, 37
    bltu a2, a3, falla          # sin signo -7 es un numero grande
    bltu a3, a2, 1f
    j    falla
1:  li   s11, 38
    bgeu a3, a2, falla
    bgeu a2, a3, 1f
    j    falla
1:

    # --- jal y jalr ---
    jal  ra, funcion            # funcion devuelve 5 en t0
    CHECK t0, 5, 39
    la   t1, destino
    addi t1, t1, 1              # el bit 0 se ignora en jalr
    jalr t2, 0(t1)
    j    falla40
destino:
    la   t1, destino
    jalr t1, 16(t1)             # rd = rs1: primero se calcula el destino
    j    falla41
    sub  t1, t1, t2             # t1 y t2 son pc + 4 de cada jalr
    CHECK t1, 16, 42

    # --- x0 siempre vale 0 ---
    addi zero, zero, 5
    CHECK zero, 0, 43

    li   a0, 1
    li   a1, 0
fin:
    j    fin

funcion:
    li   t0, 5
    jalr zero, 0(ra)

falla4:  li s11, 4
         j  falla
falla40: li s11, 40
         j  falla
falla41: li s11, 41
falla:
    li   a0, 0
    mv   a1, s11
    j    fin
