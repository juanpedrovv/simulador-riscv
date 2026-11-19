#pragma once
#include <cstdint>
#include <string>
#include "memoria.h"

// Carga un programa en memoria y devuelve el PC inicial.
//  - Binario crudo (.bin): se copia tal cual desde la direccion 0x00000000.
//  - Ejecutable ELF de 32 bits: se copian sus segmentos y se empieza en su entry point.
// En "tipo" deja una descripcion de lo que se cargo. Si falla lanza std::runtime_error.
uint32_t cargar_programa(const std::string &ruta, Memoria &mem, std::string &tipo);
