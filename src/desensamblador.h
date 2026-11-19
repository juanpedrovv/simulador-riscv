#pragma once
#include <cstdint>
#include <string>

// Traduce una instruccion de codigo maquina a assembly (seccion 6.4.7 del libro).
// Se necesita el pc para mostrar la direccion destino de branches y jal.
std::string desensamblar(uint32_t bits, uint32_t pc);
