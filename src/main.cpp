// Simulador del ISA RISC-V de 32 bits (RV32I) para la Tarea 5.
// Uso: ./build/rvsim programa.bin   (o programa.elf)
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "cargador.h"
#include "cpu.h"
#include "desensamblador.h"

using namespace std;

static const long LIMITE_RUN = 10000000;   // por si el programa nunca termina

static string hex8(uint32_t v) {
    char buf[16];
    snprintf(buf, sizeof buf, "0x%08X", v);
    return buf;
}

// Acepta decimal o hexadecimal con 0x
static bool numero(const string &s, uint32_t &v) {
    try {
        size_t usados;
        unsigned long n = stoul(s, &usados, 0);
        if (usados != s.size() || n > 0xFFFFFFFFul) return false;
        v = uint32_t(n);
        return true;
    } catch (...) {
        return false;
    }
}

// Acepta x0..x31 o el nombre ABI (a0, sp, ...). fp es lo mismo que s0.
static int registro(const string &s) {
    if (s == "fp") return 8;
    for (int i = 0; i < 32; i++)
        if (s == NOMBRES[i] || s == "x" + to_string(i)) return i;
    return -1;
}

static void mostrar_registro(const CPU &cpu, int i) {
    char buf[64];
    snprintf(buf, sizeof buf, "x%-2d (%-4s) = 0x%08X", i, NOMBRES[i], cpu.x[i]);
    cout << buf;
}

static void ayuda() {
    cout << "Comandos:\n"
            "  step [n]          ejecuta n instrucciones (1 por defecto) y muestra los cambios\n"
            "  run               ejecuta hasta que el programa termine o llegue a un breakpoint\n"
            "  pc                muestra el PC\n"
            "  regs [r1 r2 ...]  muestra todos los registros o solo los indicados (x5, a0, sp...)\n"
            "  mem ini [fin]     muestra los bytes de memoria entre ini y fin\n"
            "  memw dir [n]      muestra n palabras de 32 bits desde dir (en hex y decimal)\n"
            "  dis [dir] [n]     desensambla n instrucciones desde dir (por defecto desde el PC)\n"
            "  break dir         pone un breakpoint        delete dir   lo quita\n"
            "  breaks            lista los breakpoints\n"
            "  reset             vuelve a cargar el programa\n"
            "  help              muestra esta ayuda\n"
            "  exit              sale del simulador\n"
            "  (Enter vacío repite el último comando)\n";
}

static bool cargar(CPU &cpu, const string &ruta) {
    cpu.reiniciar();
    try {
        string tipo;
        cpu.pc = cargar_programa(ruta, cpu.mem, tipo);
        cout << "\"" << ruta << "\" cargado a memoria (" << tipo << ").\n";
        return true;
    } catch (const exception &e) {
        cout << "Error: " << e.what() << "\n";
        return false;
    }
}

// Si una ecall dejo texto sin salto de linea, se agrega uno para que no se mezcle
static void terminar_linea(CPU &cpu) {
    if (cpu.ultimo_char != '\n') {
        cout << "\n";
        cpu.ultimo_char = '\n';
    }
}

// Ejecuta una instruccion mostrando que hizo. Devuelve false si hubo error.
static bool paso_con_detalle(CPU &cpu) {
    uint32_t pc = cpu.pc;
    uint32_t bits = cpu.mem.leer32(pc);
    uint32_t antes[32];
    for (int i = 0; i < 32; i++) antes[i] = cpu.x[i];

    char buf[64];
    snprintf(buf, sizeof buf, "0x%08X:  %08X   ", pc, bits);
    cout << buf << desensamblar(bits, pc) << "\n";
    try {
        cpu.paso();
    } catch (const exception &e) {
        terminar_linea(cpu);
        cout << "Error: " << e.what() << "\n";
        return false;
    }
    terminar_linea(cpu);
    for (int i = 0; i < 32; i++) {
        if (cpu.x[i] != antes[i]) {
            cout << "    ";
            mostrar_registro(cpu, i);
            cout << "\n";
        }
    }
    if (cpu.hubo_store)
        cout << "    Mem[" << hex8(cpu.store_dir) << "] <- " << hex8(cpu.store_valor)
             << " (" << cpu.store_bytes << " byte" << (cpu.store_bytes > 1 ? "s" : "") << ")\n";
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Uso: " << argv[0] << " programa.bin|programa.elf\n";
        return 1;
    }
    string ruta = argv[1];
    CPU cpu;
    if (!cargar(cpu, ruta)) return 1;
    cout << "Escribe help para ver los comandos.\n";

    set<uint32_t> breakpoints;
    string linea, ultima;
    while (true) {
        cout << "> " << flush;
        if (!leer_linea(linea)) break;
        if (linea.empty()) linea = ultima;
        ultima = linea;

        istringstream ss(linea);
        vector<string> t;
        string palabra;
        while (ss >> palabra) t.push_back(palabra);
        if (t.empty()) continue;
        string cmd = t[0];

        if (cmd == "exit" || cmd == "quit" || cmd == "q") {
            break;

        } else if (cmd == "help") {
            ayuda();

        } else if (cmd == "pc") {
            cout << "pc = " << hex8(cpu.pc) << "\n";

        } else if (cmd == "step" || cmd == "s") {
            uint32_t n = 1;
            if (t.size() > 1 && !numero(t[1], n)) { cout << "Uso: step [n]\n"; continue; }
            for (uint32_t i = 0; i < n; i++) {
                if (cpu.terminado) break;
                if (!paso_con_detalle(cpu)) break;
            }
            if (cpu.terminado) cout << "Programa terminado: " << cpu.motivo << ".\n";

        } else if (cmd == "run" || cmd == "r") {
            if (cpu.terminado) { cout << "Programa terminado: " << cpu.motivo << ".\n"; continue; }
            long ejecutadas = 0;
            try {
                while (!cpu.terminado && ejecutadas < LIMITE_RUN) {
                    // no se para en el breakpoint del que se esta saliendo
                    if (ejecutadas > 0 && breakpoints.count(cpu.pc)) break;
                    cpu.paso();
                    ejecutadas++;
                }
            } catch (const exception &e) {
                terminar_linea(cpu);
                cout << "Error: " << e.what() << "\n";
            }
            terminar_linea(cpu);
            cout << "Se ejecutaron " << ejecutadas << " instrucciones.\n";
            if (cpu.terminado) cout << "Programa terminado: " << cpu.motivo << ".\n";
            else if (breakpoints.count(cpu.pc)) cout << "Breakpoint en " << hex8(cpu.pc) << ".\n";
            else if (ejecutadas == LIMITE_RUN) cout << "Se llegó al límite de instrucciones.\n";

        } else if (cmd == "regs") {
            if (t.size() == 1) {
                cout << "pc = " << hex8(cpu.pc) << "\n";
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 4; j++) {
                        mostrar_registro(cpu, i + 8 * j);
                        cout << (j < 3 ? "    " : "\n");
                    }
                }
            }
            for (size_t k = 1; k < t.size(); k++) {
                int i = registro(t[k]);
                if (t[k] == "pc") cout << "pc = " << hex8(cpu.pc) << "\n";
                else if (i < 0) cout << t[k] << ": no es un registro\n";
                else { mostrar_registro(cpu, i); cout << "  (" << int32_t(cpu.x[i]) << ")\n"; }
            }

        } else if (cmd == "mem") {
            uint32_t ini, fin;
            if (t.size() < 2 || !numero(t[1], ini)) { cout << "Uso: mem ini [fin]\n"; continue; }
            fin = ini + 15;
            if (t.size() > 2 && !numero(t[2], fin)) { cout << "Uso: mem ini [fin]\n"; continue; }
            if (fin < ini || fin - ini >= 4096) { cout << "Rango no válido (máximo 4096 bytes).\n"; continue; }
            if (fin - ini < 16) {
                cout << "Memoria (" << hex8(ini) << "-" << hex8(fin) << "):";
                for (uint32_t d = ini; d <= fin; d++) {
                    char buf[8];
                    snprintf(buf, sizeof buf, " %02X", cpu.mem.leer8(d));
                    cout << buf;
                }
                cout << "\n";
            } else {
                for (uint32_t d = ini; d <= fin && d >= ini; d++) {   // d >= ini por si da la vuelta
                    if (d == ini || d % 16 == 0) cout << (d == ini ? "" : "\n") << hex8(d) << ":";
                    char buf[8];
                    snprintf(buf, sizeof buf, " %02X", cpu.mem.leer8(d));
                    cout << buf;
                }
                cout << "\n";
            }

        } else if (cmd == "memw") {
            uint32_t dir, n = 1;
            if (t.size() < 2 || !numero(t[1], dir) || (t.size() > 2 && !numero(t[2], n))) {
                cout << "Uso: memw dir [n]\n";
                continue;
            }
            for (uint32_t i = 0; i < n && i < 1024; i++) {
                uint32_t v = cpu.mem.leer32(dir + 4 * i);
                cout << "Mem[" << hex8(dir + 4 * i) << "] = " << hex8(v) << "  (" << int32_t(v) << ")\n";
            }

        } else if (cmd == "dis") {
            uint32_t dir = cpu.pc, n = 8;
            if ((t.size() > 1 && !numero(t[1], dir)) || (t.size() > 2 && !numero(t[2], n))) {
                cout << "Uso: dis [dir] [n]\n";
                continue;
            }
            dir &= ~3u;
            for (uint32_t i = 0; i < n && i < 1024; i++) {
                uint32_t d = dir + 4 * i, bits = cpu.mem.leer32(d);
                char buf[64];
                snprintf(buf, sizeof buf, "%s%c 0x%08X:  %08X   ", d == cpu.pc ? "=>" : "  ",
                         breakpoints.count(d) ? '*' : ' ', d, bits);
                cout << buf << desensamblar(bits, d) << "\n";
            }

        } else if (cmd == "break" || cmd == "delete") {
            uint32_t dir;
            if (t.size() < 2 || !numero(t[1], dir)) { cout << "Uso: " << cmd << " dir\n"; continue; }
            if (cmd == "break") {
                breakpoints.insert(dir);
                cout << "Breakpoint en " << hex8(dir) << ": " << desensamblar(cpu.mem.leer32(dir), dir) << "\n";
            } else if (breakpoints.erase(dir)) {
                cout << "Breakpoint en " << hex8(dir) << " eliminado.\n";
            } else {
                cout << "No hay breakpoint en " << hex8(dir) << ".\n";
            }

        } else if (cmd == "breaks") {
            if (breakpoints.empty()) cout << "No hay breakpoints.\n";
            for (uint32_t d : breakpoints) cout << "  " << hex8(d) << "\n";

        } else if (cmd == "reset") {
            cargar(cpu, ruta);

        } else {
            cout << "Comando desconocido: " << cmd << " (escribe help)\n";
        }
    }
    cout << "See you next time...\n";
    return cpu.codigo_salida;
}
