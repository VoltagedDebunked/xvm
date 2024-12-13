#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEMORY_SIZE (1024*1024)  // 1MB of RAM

typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint32_t registers[8];  // General purpose registers
    uint32_t ip;           // Instruction pointer
    uint32_t flags;        // CPU flags
    uint16_t cs, ds, es, ss, fs, gs;  // Segment registers
    uint32_t last_write_addr;         // Track last memory write
} CPU;

// CPU operations
void cpu_init(CPU* cpu);
void cpu_emulate_cycle(CPU* cpu);
void cpu_load_program(CPU* cpu, const char* filename);

// Memory operations
uint8_t cpu_read_byte(CPU* cpu, uint32_t address);
void cpu_write_byte(CPU* cpu, uint32_t address, uint8_t value);
uint16_t cpu_read_word(CPU* cpu, uint32_t address);
void cpu_write_word(CPU* cpu, uint32_t address, uint16_t value);
uint32_t cpu_read_dword(CPU* cpu, uint32_t address);
void cpu_write_dword(CPU* cpu, uint32_t address, uint32_t value);

#endif // CPU_H