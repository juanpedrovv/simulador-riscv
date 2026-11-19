#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

// Memoria plana de 32 bits, byte-addressable y little-endian:
// el byte menos significativo de una palabra va en la direccion mas baja.
// Para no reservar 4 GB, la memoria se guarda en paginas de 4 KB que se crean
// la primera vez que se escriben. Lo que nunca se escribio vale 0.
class Memoria {
public:
    uint8_t leer8(uint32_t dir) const {
        auto it = paginas.find(dir >> 12);
        if (it == paginas.end()) return 0;
        return it->second[dir & 0xFFF];
    }

    void escribir8(uint32_t dir, uint8_t valor) {
        std::vector<uint8_t> &pag = paginas[dir >> 12];
        if (pag.empty()) pag.resize(4096, 0);
        pag[dir & 0xFFF] = valor;
    }

    uint16_t leer16(uint32_t dir) const {
        return leer8(dir) | (leer8(dir + 1) << 8);
    }

    uint32_t leer32(uint32_t dir) const {
        return leer16(dir) | (uint32_t(leer16(dir + 2)) << 16);
    }

    void escribir16(uint32_t dir, uint16_t valor) {
        escribir8(dir, valor & 0xFF);
        escribir8(dir + 1, valor >> 8);
    }

    void escribir32(uint32_t dir, uint32_t valor) {
        escribir16(dir, valor & 0xFFFF);
        escribir16(dir + 2, valor >> 16);
    }

    void limpiar() { paginas.clear(); }

private:
    std::unordered_map<uint32_t, std::vector<uint8_t>> paginas;
};
