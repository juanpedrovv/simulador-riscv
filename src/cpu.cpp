#include "cpu.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace std;

const char *NOMBRES[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

static string hex8(uint32_t v) {
    char buf[16];
    snprintf(buf, sizeof buf, "0x%08X", v);
    return buf;
}

bool leer_linea(string &linea) {
    if (!getline(cin, linea)) return false;
    if (!isatty(0)) cout << linea << "\n";
    return true;
}

// Separa los campos y arma el inmediato segun el tipo (Figuras 6.26 y 6.27).
// instr31 siempre es el bit de signo, por eso se usa un int32_t con >> aritmetico.
Instruccion decodificar(uint32_t b) {
    Instruccion in;
    in.bits = b;
    in.op = b & 0x7F;
    in.rd = (b >> 7) & 0x1F;
    in.funct3 = (b >> 12) & 0x7;
    in.rs1 = (b >> 15) & 0x1F;
    in.rs2 = (b >> 20) & 0x1F;
    in.funct7 = b >> 25;

    int32_t s = int32_t(b);
    uint32_t imm;
    switch (in.op) {
    case 0x23:  // tipo S: imm[11:5] = instr[31:25], imm[4:0] = instr[11:7]
        imm = (uint32_t(s >> 20) & ~0x1Fu) | ((b >> 7) & 0x1F);
        break;
    case 0x63:  // tipo B: imm[12|10:5|4:1|11], el bit 0 siempre es 0
        imm = (uint32_t(s >> 19) & 0xFFFFF000) | ((b << 4) & 0x800) |
              ((b >> 20) & 0x7E0) | ((b >> 7) & 0x1E);
        break;
    case 0x37:  // tipo U: los 20 bits de arriba
    case 0x17:
        imm = b & 0xFFFFF000;
        break;
    case 0x6F:  // tipo J: imm[20|10:1|11|19:12], el bit 0 siempre es 0
        imm = (uint32_t(s >> 11) & 0xFFF00000) | (b & 0xFF000) |
              ((b >> 9) & 0x800) | ((b >> 20) & 0x7FE);
        break;
    default:    // tipo I (y en tipo R no se usa)
        imm = uint32_t(s >> 20);
        break;
    }
    in.imm = int32_t(imm);
    return in;
}

// Excepciones de la Tabla 6.6 del libro. El simulador no tiene un exception
// handler, asi que solo se reporta el error y se detiene la ejecucion.
[[noreturn]] static void ilegal(const Instruccion &in, uint32_t pc) {
    throw runtime_error("instrucción ilegal " + hex8(in.bits) +
                        " en pc = " + hex8(pc) + " (mcause = 2)");
}

// ALU para tipo R y tipo I. alt = 1 para sub y sra/srai (funct7 = 0100000)
static uint32_t alu(uint32_t funct3, bool alt, uint32_t a, uint32_t b) {
    uint32_t shamt = b & 0x1F;   // solo se usan los 5 bits de abajo
    switch (funct3) {
    case 0: return alt ? a - b : a + b;                    // add / sub
    case 1: return a << shamt;                             // sll
    case 2: return int32_t(a) < int32_t(b) ? 1 : 0;        // slt
    case 3: return a < b ? 1 : 0;                          // sltu
    case 4: return a ^ b;                                  // xor
    case 5: return alt ? uint32_t(int32_t(a) >> shamt)     // sra
                       : a >> shamt;                       // srl
    case 6: return a | b;                                  // or
    default: return a & b;                                 // and
    }
}

void CPU::reiniciar() {
    pc = 0;
    for (uint32_t &r : x) r = 0;
    mem.limpiar();
    terminado = false;
    motivo = "";
    codigo_salida = 0;
    hubo_store = false;
}

void CPU::paso() {
    if (pc % 4 != 0)
        throw runtime_error("pc desalineado " + hex8(pc) + " (mcause = 0)");

    // Fetch y Decode
    Instruccion in = decodificar(mem.leer32(pc));
    uint32_t a = x[in.rs1];
    uint32_t b = x[in.rs2];
    uint32_t pc_sig = pc + 4;
    uint32_t resultado = 0;
    bool escribe_rd = false;
    hubo_store = false;

    // Execute (opcodes de la Tabla B.1)
    switch (in.op) {
    case 0x37:  // lui
        resultado = in.imm;
        escribe_rd = true;
        break;

    case 0x17:  // auipc
        resultado = pc + in.imm;
        escribe_rd = true;
        break;

    case 0x6F:  // jal
        resultado = pc + 4;
        escribe_rd = true;
        pc_sig = pc + in.imm;
        break;

    case 0x67:  // jalr (el bit 0 de la direccion se pone en 0)
        if (in.funct3 != 0) ilegal(in, pc);
        resultado = pc + 4;
        escribe_rd = true;
        pc_sig = (a + in.imm) & ~1u;
        break;

    case 0x63: {  // branches
        bool salta;
        switch (in.funct3) {
        case 0: salta = a == b; break;                          // beq
        case 1: salta = a != b; break;                          // bne
        case 4: salta = int32_t(a) < int32_t(b); break;         // blt
        case 5: salta = int32_t(a) >= int32_t(b); break;        // bge
        case 6: salta = a < b; break;                           // bltu
        case 7: salta = a >= b; break;                          // bgeu
        default: ilegal(in, pc);
        }
        if (salta) pc_sig = pc + in.imm;
        break;
    }

    case 0x03: {  // loads
        uint32_t dir = a + in.imm;
        switch (in.funct3) {
        case 0: resultado = int32_t(int8_t(mem.leer8(dir))); break;    // lb
        case 1: resultado = int32_t(int16_t(mem.leer16(dir))); break;  // lh
        case 2: resultado = mem.leer32(dir); break;                    // lw
        case 4: resultado = mem.leer8(dir); break;                     // lbu
        case 5: resultado = mem.leer16(dir); break;                    // lhu
        default: ilegal(in, pc);
        }
        escribe_rd = true;
        break;
    }

    case 0x23: {  // stores
        uint32_t dir = a + in.imm;
        switch (in.funct3) {
        case 0: mem.escribir8(dir, b & 0xFF); store_bytes = 1; store_valor = b & 0xFF; break;
        case 1: mem.escribir16(dir, b & 0xFFFF); store_bytes = 2; store_valor = b & 0xFFFF; break;
        case 2: mem.escribir32(dir, b); store_bytes = 4; store_valor = b; break;
        default: ilegal(in, pc);
        }
        hubo_store = true;
        store_dir = dir;
        break;
    }

    case 0x13: {  // tipo I de la ALU
        bool alt = false;
        if (in.funct3 == 1 && in.funct7 != 0) ilegal(in, pc);          // slli
        if (in.funct3 == 5) {                                           // srli / srai
            if (in.funct7 != 0 && in.funct7 != 0x20) ilegal(in, pc);
            alt = in.funct7 == 0x20;
        }
        resultado = alu(in.funct3, alt, a, uint32_t(in.imm));
        escribe_rd = true;
        break;
    }

    case 0x33: {  // tipo R
        bool alt = in.funct7 == 0x20;
        if (in.funct7 != 0 && !(alt && (in.funct3 == 0 || in.funct3 == 5))) ilegal(in, pc);
        resultado = alu(in.funct3, alt, a, b);
        escribe_rd = true;
        break;
    }

    case 0x73:  // ecall / ebreak
        if (in.bits == 0x00000073) {
            ecall();
        } else if (in.bits == 0x00100073) {
            terminado = true;
            motivo = "ebreak en " + hex8(pc);
        } else {
            ilegal(in, pc);
        }
        break;

    default:
        ilegal(in, pc);
    }

    // Writeback: x0 siempre vale 0
    if (escribe_rd && in.rd != 0) x[in.rd] = resultado;

    // Un salto a si mismo (j . o beq x0, x0, .) es como terminan los programas
    if (pc_sig == pc && !terminado) {
        terminado = true;
        motivo = "bucle infinito en " + hex8(pc);
    }
    pc = pc_sig;
}

// Environment calls de SPIM (las mismas que usa CPUlator): a7 = numero, a0 = argumento
void CPU::ecall() {
    uint32_t a0 = x[10];
    string linea;
    switch (x[17]) {
    case 1:   // print int
        cout << int32_t(a0);
        ultimo_char = '0';
        break;
    case 4:   // print string (termina en 0)
        for (uint32_t d = a0; mem.leer8(d) != 0; d++) {
            ultimo_char = char(mem.leer8(d));
            cout << ultimo_char;
        }
        break;
    case 5:   // read int
        if (!leer_linea(linea)) throw runtime_error("ecall read int: no hay entrada");
        ultimo_char = '\n';
        try {
            x[10] = uint32_t(stol(linea));
        } catch (...) {
            throw runtime_error("ecall read int: \"" + linea + "\" no es un número");
        }
        break;
    case 8: { // read string: a0 = buffer, a1 = tamano maximo
        if (!leer_linea(linea)) throw runtime_error("ecall read string: no hay entrada");
        ultimo_char = '\n';
        uint32_t max = x[11];
        uint32_t i = 0;
        for (; i + 1 < max && i < linea.size(); i++) mem.escribir8(a0 + i, linea[i]);
        if (max > 0) mem.escribir8(a0 + i, 0);
        break;
    }
    case 10:  // exit
        terminado = true;
        motivo = "ecall exit";
        break;
    case 11:  // print char
        ultimo_char = char(a0 & 0xFF);
        cout << ultimo_char;
        break;
    case 12:  // read char
        if (!leer_linea(linea)) throw runtime_error("ecall read char: no hay entrada");
        ultimo_char = '\n';
        x[10] = linea.empty() ? '\n' : uint8_t(linea[0]);
        break;
    case 17:  // exit2 con codigo en a0
        terminado = true;
        codigo_salida = int32_t(a0);
        motivo = "ecall exit con código " + to_string(codigo_salida);
        break;
    case 34:  // print int en hexadecimal
        cout << hex8(a0);
        ultimo_char = '0';
        break;
    case 36:  // print int sin signo
        cout << a0;
        ultimo_char = '0';
        break;
    default:
        throw runtime_error("ecall no soportada: a7 = " + to_string(x[17]));
    }
    cout.flush();
}
