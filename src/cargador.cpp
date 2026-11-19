#include "cargador.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

using namespace std;

// Lectura little-endian de los campos del archivo ELF
static uint32_t campo(const vector<uint8_t> &d, size_t pos, int bytes) {
    if (pos + bytes > d.size()) throw runtime_error("archivo ELF incompleto");
    uint32_t v = 0;
    for (int i = bytes - 1; i >= 0; i--) v = (v << 8) | d[pos + i];
    return v;
}

// Del ELF solo nos interesan los segmentos que se cargan (PT_LOAD), que son el
// text y el data segment del ejecutable, y el entry point para el PC.
static uint32_t cargar_elf(const vector<uint8_t> &d, Memoria &mem, string &tipo) {
    if (d.size() < 52 || d[4] != 1 || d[5] != 1)
        throw runtime_error("solo se soportan ELF de 32 bits little-endian");
    if (campo(d, 18, 2) != 0xF3)
        throw runtime_error("el ELF no es de RISC-V");

    uint32_t entrada = campo(d, 24, 4);
    uint32_t phoff = campo(d, 28, 4);
    uint32_t phentsize = campo(d, 42, 2);
    uint32_t phnum = campo(d, 44, 2);

    for (uint32_t i = 0; i < phnum; i++) {
        size_t ph = phoff + size_t(i) * phentsize;
        if (campo(d, ph, 4) != 1) continue;    // 1 = PT_LOAD
        uint32_t offset = campo(d, ph + 4, 4);
        uint32_t vaddr = campo(d, ph + 8, 4);
        uint32_t filesz = campo(d, ph + 16, 4);
        uint32_t memsz = campo(d, ph + 20, 4);
        if (size_t(offset) + filesz > d.size()) throw runtime_error("segmento fuera del archivo");
        for (uint32_t j = 0; j < memsz; j++)
            mem.escribir8(vaddr + j, j < filesz ? d[offset + j] : 0);
    }

    char buf[80];
    snprintf(buf, sizeof buf, "ELF, entry point 0x%08X", entrada);
    tipo = buf;
    return entrada;
}

uint32_t cargar_programa(const string &ruta, Memoria &mem, string &tipo) {
    ifstream f(ruta, ios::binary);
    if (!f) throw runtime_error("no se pudo abrir \"" + ruta + "\"");
    vector<uint8_t> datos((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    if (datos.size() >= 4 && datos[0] == 0x7F && datos[1] == 'E' && datos[2] == 'L' && datos[3] == 'F')
        return cargar_elf(datos, mem, tipo);

    // binario crudo desde la direccion 0
    for (size_t i = 0; i < datos.size(); i++) mem.escribir8(uint32_t(i), datos[i]);
    tipo = "binario crudo, " + to_string(datos.size()) + " bytes";
    return 0;
}
