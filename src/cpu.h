#pragma once
#include <cstdint>
#include <string>
#include "memoria.h"

// Nombres de los registros (Tabla B.4 del libro)
extern const char *NOMBRES[32];

// Campos de una instruccion, igual que en la etapa Decode
struct Instruccion {
    uint32_t bits;
    uint32_t op, rd, funct3, rs1, rs2, funct7;
    int32_t imm;    // inmediato ya extendido en signo segun el tipo
};

Instruccion decodificar(uint32_t bits);

// Lee una linea de la entrada. Si la entrada no es una terminal (por ejemplo
// un archivo con comandos) la vuelve a imprimir para que se vea en la salida.
bool leer_linea(std::string &linea);

// Estado arquitectural: PC, 32 registros y memoria
class CPU {
public:
    uint32_t pc = 0;
    uint32_t x[32] = {};
    Memoria mem;

    bool terminado = false;
    std::string motivo;        // por que termino el programa
    int codigo_salida = 0;
    char ultimo_char = '\n';  // ultimo caracter que imprimio una ecall

    // ultimo store, para mostrarlo con step
    bool hubo_store = false;
    uint32_t store_dir = 0, store_valor = 0, store_bytes = 0;

    void reiniciar();
    // Ejecuta la instruccion apuntada por el PC.
    // Si no se puede ejecutar lanza std::runtime_error y el estado no cambia.
    void paso();

private:
    void ecall();
};
