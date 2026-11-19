#include "desensamblador.h"
#include "cpu.h"
#include <cstdio>

using namespace std;

static string r(uint32_t n) { return NOMBRES[n]; }

static string hex(uint32_t v, int digitos) {
    char buf[16];
    snprintf(buf, sizeof buf, "0x%0*X", digitos, v);
    return buf;
}

// mnemonico con espacios a la derecha para que las columnas queden alineadas
static string fmt(const string &mnemonico, const string &operandos) {
    string s = mnemonico;
    while (s.size() < 7) s += ' ';
    return s + operandos;
}

string desensamblar(uint32_t bits, uint32_t pc) {
    Instruccion in = decodificar(bits);
    string rd = r(in.rd), rs1 = r(in.rs1), rs2 = r(in.rs2);
    string imm = to_string(in.imm);

    switch (in.op) {
    case 0x37:
        return fmt("lui", rd + ", " + hex(uint32_t(in.imm) >> 12, 1));
    case 0x17:
        return fmt("auipc", rd + ", " + hex(uint32_t(in.imm) >> 12, 1));
    case 0x6F:
        return fmt("jal", rd + ", " + hex(pc + in.imm, 8));
    case 0x67:
        if (in.funct3 == 0) return fmt("jalr", rd + ", " + imm + "(" + rs1 + ")");
        break;
    case 0x63: {
        const char *nombres[8] = {"beq", "bne", "", "", "blt", "bge", "bltu", "bgeu"};
        if (in.funct3 == 2 || in.funct3 == 3) break;
        return fmt(nombres[in.funct3], rs1 + ", " + rs2 + ", " + hex(pc + in.imm, 8));
    }
    case 0x03: {
        const char *nombres[8] = {"lb", "lh", "lw", "", "lbu", "lhu", "", ""};
        if (nombres[in.funct3][0] == 0) break;
        return fmt(nombres[in.funct3], rd + ", " + imm + "(" + rs1 + ")");
    }
    case 0x23: {
        const char *nombres[3] = {"sb", "sh", "sw"};
        if (in.funct3 > 2) break;
        return fmt(nombres[in.funct3], rs2 + ", " + imm + "(" + rs1 + ")");
    }
    case 0x13: {
        const char *nombres[8] = {"addi", "slli", "slti", "sltiu", "xori", "srli", "ori", "andi"};
        if (in.funct3 == 1 || in.funct3 == 5) {   // shifts: el inmediato es shamt
            string m = nombres[in.funct3];
            if (in.funct3 == 5 && in.funct7 == 0x20) m = "srai";
            else if (in.funct7 != 0) break;
            return fmt(m, rd + ", " + rs1 + ", " + to_string(in.rs2));
        }
        return fmt(nombres[in.funct3], rd + ", " + rs1 + ", " + imm);
    }
    case 0x33: {
        const char *nombres[8] = {"add", "sll", "slt", "sltu", "xor", "srl", "or", "and"};
        string m = nombres[in.funct3];
        if (in.funct7 == 0x20 && in.funct3 == 0) m = "sub";
        else if (in.funct7 == 0x20 && in.funct3 == 5) m = "sra";
        else if (in.funct7 != 0) break;
        return fmt(m, rd + ", " + rs1 + ", " + rs2);
    }
    case 0x73:
        if (bits == 0x00000073) return "ecall";
        if (bits == 0x00100073) return "ebreak";
        break;
    }
    return "(inválida)";
}
