#include <stdio.h>
#include <string.h>
#include <cpu.h>

void cpu_init(CPU* cpu) {
    memset(cpu, 0, sizeof(CPU));
}

uint8_t cpu_read_byte(CPU* cpu, uint32_t address) {
    if (address < MEMORY_SIZE) {
        return cpu->memory[address];
    }
    return 0;
}

void cpu_write_byte(CPU* cpu, uint32_t address, uint8_t value) {
    if (address < MEMORY_SIZE) {
        cpu->memory[address] = value;
    }
}

uint32_t cpu_read_dword(CPU* cpu, uint32_t address) {
    if (address + 3 < MEMORY_SIZE) {
        return *(uint32_t*)&cpu->memory[address];
    }
    return 0;
}

void cpu_write_dword(CPU* cpu, uint32_t address, uint32_t value) {
    if (address + 3 < MEMORY_SIZE) {
        *(uint32_t*)&cpu->memory[address] = value;
    }
}

void cpu_write_word(CPU* cpu, uint32_t address, uint16_t value) {
    if (address + 1 < MEMORY_SIZE) {
        cpu->memory[address] = value & 0xFF;
        cpu->memory[address + 1] = (value >> 8) & 0xFF;
    }
}

uint16_t cpu_read_word(CPU* cpu, uint32_t address) {
    if (address + 1 < MEMORY_SIZE) {
        return cpu->memory[address] | (cpu->memory[address + 1] << 8);
    }
    return 0;
}

void cpu_emulate_cycle(CPU* cpu) {
    uint8_t opcode = cpu_read_byte(cpu, cpu->ip);
    uint8_t reg1, reg2, modrm;
    int8_t offset;
    uint32_t addr, value;
    
    switch(opcode) {
        // 0x00-0x0F: Basic Data Movement
        case 0x00: // NOP
            cpu->ip++;
            break;
            
        case 0x01: // MOV reg, imm32
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) cpu->registers[reg1] = value;
            cpu->ip += 6;
            break;
            
        case 0x02: // MOV [addr], reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            addr = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) cpu_write_dword(cpu, addr, cpu->registers[reg1]);
            cpu->ip += 6;
            break;
            
        case 0x03: // MOV reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) cpu->registers[reg1] = cpu->registers[reg2];
            cpu->ip += 3;
            break;
            
        case 0x04: // MOV reg, [addr]
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            addr = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) cpu->registers[reg1] = cpu_read_dword(cpu, addr);
            cpu->ip += 6;
            break;
            
        case 0x05: // XCHG reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                value = cpu->registers[reg1];
                cpu->registers[reg1] = cpu->registers[reg2];
                cpu->registers[reg2] = value;
            }
            cpu->ip += 3;
            break;

        case 0x06: // PUSH reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[7] -= 4;
                cpu_write_dword(cpu, cpu->registers[7], cpu->registers[reg1]);
            }
            cpu->ip += 2;
            break;

        case 0x07: // POP reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[reg1] = cpu_read_dword(cpu, cpu->registers[7]);
                cpu->registers[7] += 4;
            }
            cpu->ip += 2;
            break;

        // 0x10-0x1F: Advanced Data Movement
        case 0x10: // MOVZX reg1, byte[addr]
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            addr = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) cpu->registers[reg1] = cpu_read_byte(cpu, addr);
            cpu->ip += 6;
            break;

        case 0x11: // MOVSX reg1, byte[addr]
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            addr = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                int8_t signed_byte = (int8_t)cpu_read_byte(cpu, addr);
                cpu->registers[reg1] = (int32_t)signed_byte;
            }
            cpu->ip += 6;
            break;

        // 0x20-0x2F: Basic Arithmetic
        case 0x20: // ADD reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] += cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x21: // ADD reg, imm32
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                cpu->registers[reg1] += value;
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 6;
            break;

        case 0x22: // SUB reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] -= cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x23: // MUL reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                uint64_t result = (uint64_t)cpu->registers[0] * cpu->registers[reg1];
                cpu->registers[0] = result & 0xFFFFFFFF;
                cpu->registers[1] = (result >> 32) & 0xFFFFFFFF;
            }
            cpu->ip += 2;
            break;

        case 0x24: // DIV reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8 && cpu->registers[reg1] != 0) {
                uint64_t dividend = ((uint64_t)cpu->registers[1] << 32) | cpu->registers[0];
                cpu->registers[0] = dividend / cpu->registers[reg1];  // quotient
                cpu->registers[1] = dividend % cpu->registers[reg1];  // remainder
            }
            cpu->ip += 2;
            break;

        // 0x30-0x3F: Advanced Arithmetic
        case 0x30: // INC reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[reg1]++;
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 2;
            break;

        case 0x31: // DEC reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[reg1]--;
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 2;
            break;

        case 0x32: // NEG reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[reg1] = -cpu->registers[reg1];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 2;
            break;

        // 0x40-0x4F: Bitwise Operations
        case 0x40: // AND reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] &= cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x41: // OR reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] |= cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x42: // XOR reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] ^= cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x43: // NOT reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[reg1] = ~cpu->registers[reg1];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 2;
            break;

        case 0x44: // SHL reg, imm8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                cpu->registers[reg1] <<= value;
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x45: // SHR reg, imm8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                cpu->registers[reg1] >>= value;
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x46: // ROL reg, imm8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_byte(cpu, cpu->ip + 2) & 0x1F;
            if (reg1 < 8) {
                uint32_t x = cpu->registers[reg1];
                cpu->registers[reg1] = (x << value) | (x >> (32 - value));
            }
            cpu->ip += 3;
            break;

        case 0x47: // ROR reg, imm8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_byte(cpu, cpu->ip + 2) & 0x1F;
            if (reg1 < 8) {
                uint32_t x = cpu->registers[reg1];
                cpu->registers[reg1] = (x >> value) | (x << (32 - value));
            }
            cpu->ip += 3;
            break;

        // 0x50-0x5F: Comparison and Jumps
        case 0x50: // CMP reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                uint32_t result = cpu->registers[reg1] - cpu->registers[reg2];
                cpu->flags = 0;
                if (result == 0) cpu->flags |= 1;  // ZF
                if (cpu->registers[reg1] < cpu->registers[reg2]) cpu->flags |= 2;  // CF
            }
            cpu->ip += 3;
            break;

        case 0x51: // JMP imm32
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            cpu->ip = addr;
            break;

        case 0x52: // JZ imm32
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            if (cpu->flags & 1) cpu->ip = addr;
            else cpu->ip += 5;
            break;

        case 0x53: // JNZ imm32
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            if (!(cpu->flags & 1)) cpu->ip = addr;
            else cpu->ip += 5;
            break;

        case 0x54: // JA imm32 (above - unsigned)
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            if (!(cpu->flags & 3)) cpu->ip = addr;  // !CF and !ZF
            else cpu->ip += 5;
            break;

        case 0x55: // JB imm32 (below - unsigned)
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            if (cpu->flags & 2) cpu->ip = addr;  // CF
            else cpu->ip += 5;
            break;

        // 0x60-0x6F: Subroutines and Stack
        case 0x60: // CALL imm32
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            cpu->registers[7] -= 4;
            cpu_write_dword(cpu, cpu->registers[7], cpu->ip + 5);
            cpu->ip = addr;
            break;

        case 0x61: // RET
            addr = cpu_read_dword(cpu, cpu->registers[7]);
            cpu->registers[7] += 4;
            cpu->ip = addr;
            break;

        case 0x62: // PUSHF
            cpu->registers[7] -= 4;
            cpu_write_dword(cpu, cpu->registers[7], cpu->flags);
            cpu->ip++;
            break;

        case 0x63: // POPF
            cpu->flags = cpu_read_dword(cpu, cpu->registers[7]);
            cpu->registers[7] += 4;
            cpu->ip++;
            break;

        // 0x70-0x7F: Advanced Flow Control
        case 0x70: // LOOP imm32
            addr = cpu_read_dword(cpu, cpu->ip + 1);
            cpu->registers[2]--;  // Assume R2 is counter
            if (cpu->registers[2] != 0)
                        addr = cpu_read_dword(cpu, cpu->ip + 1);
            cpu->registers[2]--;  // Assume R2 is counter
            if (cpu->registers[2] != 0) cpu->ip = addr;
            else cpu->ip += 5;
            break;

        // 0x80-0x8F: Memory Operations
        case 0x80: // LEA reg, [addr]
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            addr = cpu_read_dword(cpu, cpu->ip + 2);
            if (reg1 < 8) cpu->registers[reg1] = addr;
            cpu->ip += 6;
            break;

        case 0x81: // CMPSB
            {
                uint8_t val1 = cpu_read_byte(cpu, cpu->registers[4]);  // SI in R4
                uint8_t val2 = cpu_read_byte(cpu, cpu->registers[5]);  // DI in R5
                cpu->registers[4]++;
                cpu->registers[5]++;
                cpu->flags = (val1 == val2) ? 1 : 0;
            }
            cpu->ip++;
            break;

        case 0x82: // MOVSB
            {
                uint8_t val = cpu_read_byte(cpu, cpu->registers[4]);  // SI in R4
                cpu_write_byte(cpu, cpu->registers[5], val);          // DI in R5
                cpu->registers[4]++;
                cpu->registers[5]++;
            }
            cpu->ip++;
            break;

        // 0x90-0x9F: String Operations
        case 0x90: // REP prefix
            {
                uint8_t next_op = cpu_read_byte(cpu, cpu->ip + 1);
                while (cpu->registers[2] != 0) {  // CX in R2
                    switch (next_op) {
                        case 0x81: // REP CMPSB
                            {
                                uint8_t val1 = cpu_read_byte(cpu, cpu->registers[4]);
                                uint8_t val2 = cpu_read_byte(cpu, cpu->registers[5]);
                                cpu->registers[4]++;
                                cpu->registers[5]++;
                                cpu->flags = (val1 == val2) ? 1 : 0;
                                if (val1 != val2) goto rep_done;
                            }
                            break;
                        case 0x82: // REP MOVSB
                            {
                                uint8_t val = cpu_read_byte(cpu, cpu->registers[4]);
                                cpu_write_byte(cpu, cpu->registers[5], val);
                                cpu->registers[4]++;
                                cpu->registers[5]++;
                            }
                            break;
                    }
                    cpu->registers[2]--;
                }
                rep_done:
                cpu->ip += 2;
            }
            break;

        // 0xA0-0xAF: I/O Operations
        case 0xA0: // IN reg, port
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            value = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                // Simulate I/O - in real VM would interface with devices
                cpu->registers[reg1] = 0xFF;  // Dummy read
            }
            cpu->ip += 3;
            break;

        case 0xA1: // OUT port, reg
            value = cpu_read_byte(cpu, cpu->ip + 1);
            reg1 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8) {
                // Simulate I/O - in real VM would interface with devices
                // printf("Output %08X to port %02X\n", cpu->registers[reg1], value);
            }
            cpu->ip += 3;
            break;

        // 0xB0-0xBF: System Operations
        case 0xB0: // CLI - Clear Interrupt Flag
            cpu->flags &= ~4;  // Bit 2 is IF
            cpu->ip++;
            break;

        case 0xB1: // STI - Set Interrupt Flag
            cpu->flags |= 4;   // Bit 2 is IF
            cpu->ip++;
            break;

        case 0xB2: // HLT
            // In real implementation would signal VM to stop
            printf("CPU HALT\n");
            cpu->ip++;
            break;

        // 0xC0-0xCF: Extended Arithmetic
        case 0xC0: // IMUL reg1, reg2
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                int64_t result = (int64_t)(int32_t)cpu->registers[reg1] * 
                                (int64_t)(int32_t)cpu->registers[reg2];
                cpu->registers[reg1] = (uint32_t)result;
                cpu->registers[1] = (uint32_t)(result >> 32);  // High part in rdx
            }
            cpu->ip += 3;
            break;

        case 0xC1: // IDIV reg
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8 && cpu->registers[reg1] != 0) {
                int64_t dividend = ((int64_t)cpu->registers[1] << 32) | cpu->registers[0];
                int32_t divisor = (int32_t)cpu->registers[reg1];
                cpu->registers[0] = dividend / divisor;  // quotient
                cpu->registers[1] = dividend % divisor;  // remainder
            }
            cpu->ip += 2;
            break;

        // 0xD0-0xDF: Extended Bit Operations
        case 0xD0: // BSF reg1, reg2 (Bit Scan Forward)
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                uint32_t value = cpu->registers[reg2];
                if (value == 0) {
                    cpu->flags |= 1;  // ZF = 1
                } else {
                    cpu->flags &= ~1;  // ZF = 0
                    cpu->registers[reg1] = __builtin_ctz(value);
                }
            }
            cpu->ip += 3;
            break;

        case 0xD1: // BSR reg1, reg2 (Bit Scan Reverse)
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                uint32_t value = cpu->registers[reg2];
                if (value == 0) {
                    cpu->flags |= 1;  // ZF = 1
                } else {
                    cpu->flags &= ~1;  // ZF = 0
                    cpu->registers[reg1] = 31 - __builtin_clz(value);
                }
            }
            cpu->ip += 3;
            break;

        case 0xD2: // POPCNT reg1, reg2 (Population Count)
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] = __builtin_popcount(cpu->registers[reg2]);
            }
            cpu->ip += 3;
            break;

        // 0xE0-0xEF: Extended Flow Control
        case 0xE0: // SYSCALL
            // Save return address
            cpu->registers[7] -= 4;
            cpu_write_dword(cpu, cpu->registers[7], cpu->ip + 1);
            // Save flags
            cpu->registers[7] -= 4;
            cpu_write_dword(cpu, cpu->registers[7], cpu->flags);
            // Jump to system call handler (fixed address for simplicity)
            cpu->ip = 0x1000;  // System call table address
            break;

        case 0xE1: // SYSRET
            // Restore flags
            cpu->flags = cpu_read_dword(cpu, cpu->registers[7]);
            cpu->registers[7] += 4;
            // Restore return address
            cpu->ip = cpu_read_dword(cpu, cpu->registers[7]);
            cpu->registers[7] += 4;
            break;

        // 0xF0-0xFF: Special Instructions
        case 0xF0: // CPUID
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            switch (cpu->registers[0]) {  // EAX has function number
                case 0:  // Maximum supported function
                    cpu->registers[0] = 1;  // Only basic functions
                    cpu->registers[1] = 0x756E694C;  // "Linu"
                    cpu->registers[2] = 0x20782D78;  // "x-x "
                    cpu->registers[3] = 0x20202020;  // "    "
                    break;
                case 1:  // Feature bits
                    cpu->registers[0] = 0x000000F1;  // Some CPU features
                    cpu->registers[1] = 0;           // No additional features
                    cpu->registers[2] = 0x00000001;  // SSE3 only
                    cpu->registers[3] = 0x00000001;  // FPU present
                    break;
            }
            cpu->ip += 2;
            break;

        case 0xF1: // RDTSC - Read Time Stamp Counter
            {
                static uint64_t tsc = 0;
                tsc++;
                cpu->registers[0] = (uint32_t)tsc;          // Low 32 bits in EAX
                cpu->registers[1] = (uint32_t)(tsc >> 32);  // High 32 bits in EDX
            }
            cpu->ip++;
            break;

        case 0xF2: // PAUSE - Spin-loop hint
            // In a real CPU this would delay execution slightly
            cpu->ip++;
            break;

        case 0xFF: // UD2 - Undefined instruction (guaranteed invalid)
            printf("Invalid instruction (UD2) at IP: 0x%08X\n", cpu->ip);
            cpu->ip++;
            break;

                case 0x09: // OR r/m32, r32
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] |= cpu->registers[reg2];
                cpu->flags = (cpu->registers[reg1] == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0x0F: // Two-byte opcode prefix
            opcode = cpu_read_byte(cpu, cpu->ip + 1);
            switch(opcode) {
                case 0x84: // JNLE rel32 (Jump if not less or equal)
                    addr = cpu_read_dword(cpu, cpu->ip + 2);
                    if (!(cpu->flags & 1) && !(cpu->flags & 0x80)) { // !ZF && !SF
                        cpu->ip = addr;
                    } else {
                        cpu->ip += 6;
                    }
                    break;
                    
                default:
                    printf("Unknown two-byte opcode: 0x0F 0x%02X at IP: 0x%08X\n", opcode, cpu->ip);
                    cpu->ip += 2;
            }
            break;

        case 0x49: // DEC ecx (Register 1)
            cpu->registers[1]--;
            cpu->flags = (cpu->registers[1] == 0) ? 1 : 0;
            cpu->ip++;
            break;

        case 0x6F: // OUTSD - Output doubleword to port
            {
                // Handle port output here
                cpu->registers[4] += 4;  // Increment esi
            }
            cpu->ip++;
            break;

        case 0x75: // JNE/JNZ rel8 (Jump if not equal/zero)
            offset = (int8_t)cpu_read_byte(cpu, cpu->ip + 1);
            if (!(cpu->flags & 1)) { // If zero flag is not set
                cpu->ip += offset + 2;
            } else {
                cpu->ip += 2;
            }
            break;

        case 0x84: // TEST r/m8, r8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                uint32_t result = cpu->registers[reg1] & cpu->registers[reg2];
                cpu->flags = (result == 0) ? 1 : 0;
            }
            cpu->ip += 3;
            break;

        case 0xC9: // LEAVE - Restore stack for procedure exit
            cpu->registers[7] = cpu->registers[5];  // mov esp, ebp
            cpu->registers[5] = cpu_read_dword(cpu, cpu->registers[7]);  // pop ebp
            cpu->registers[7] += 4;
            cpu->ip++;
            break;

        case 0xEB: // JMP rel8 (Short jump)
            offset = (int8_t)cpu_read_byte(cpu, cpu->ip + 1);
            cpu->ip += offset + 2;
            break;

        case 0xF6: // TEST/NOT/NEG/MUL/IMUL/DIV/IDIV r/m8
            modrm = cpu_read_byte(cpu, cpu->ip + 1);
            reg1 = modrm & 0x07;  // rm field
            switch((modrm >> 3) & 0x07) {  // reg field determines operation
                case 0: // TEST r/m8, imm8
                    value = cpu_read_byte(cpu, cpu->ip + 2);
                    if (reg1 < 8) {
                        uint32_t result = (cpu->registers[reg1] & 0xFF) & value;
                        cpu->flags = (result == 0) ? 1 : 0;
                    }
                    cpu->ip += 3;
                    break;
                    
                case 2: // NOT r/m8
                    if (reg1 < 8) {
                        cpu->registers[reg1] = (~cpu->registers[reg1] & 0xFF) | 
                                             (cpu->registers[reg1] & 0xFFFFFF00);
                    }
                    cpu->ip += 2;
                    break;
                    
                case 3: // NEG r/m8
                    if (reg1 < 8) {
                        uint8_t val = cpu->registers[reg1] & 0xFF;
                        uint8_t result = -val;
                        cpu->registers[reg1] = (cpu->registers[reg1] & 0xFFFFFF00) | result;
                        cpu->flags = (result == 0) ? 1 : 0;
                        if (val != 0) cpu->flags |= 2;  // Set CF if value wasn't 0
                    }
                    cpu->ip += 2;
                    break;
                    
                default:
                    printf("Unknown F6 group operation: %d at IP: 0x%08X\n", 
                           (modrm >> 3) & 0x07, cpu->ip);
                    cpu->ip += 2;
            }
            break;

        case 0xFA: // CLI - Clear Interrupt Flag
            cpu->flags &= ~0x200;  // Clear IF (bit 9)
            cpu->ip++;
            break;

        case 0xC2: // RET imm16 - Return and pop imm16 bytes
            {
                uint16_t pop_bytes = cpu_read_byte(cpu, cpu->ip + 1) | 
                                   (cpu_read_byte(cpu, cpu->ip + 2) << 8);
                addr = cpu_read_dword(cpu, cpu->registers[7]);  // Pop return address
                cpu->registers[7] += 4;                         // Adjust for return address
                cpu->registers[7] += pop_bytes;                 // Adjust stack by parameter
                cpu->ip = addr;
            }
            break;

        case 0x08: // OR r/m8, r8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                cpu->registers[reg1] = (cpu->registers[reg1] & 0xFFFFFF00) |
                    ((cpu->registers[reg1] | cpu->registers[reg2]) & 0xFF);
            }
            cpu->ip += 3;
            break;

        case 0x0E: // PUSH CS
            cpu->registers[7] -= 2;  // 16-bit push in real mode
            cpu_write_word(cpu, cpu->registers[7], cpu->cs);
            cpu->ip++;
            break;

        case 0x2C: // SUB AL, imm8
            value = cpu_read_byte(cpu, cpu->ip + 1);
            cpu->registers[0] = (cpu->registers[0] & 0xFFFFFF00) |
                ((cpu->registers[0] - value) & 0xFF);
            cpu->ip += 2;
            break;

        case 0x64: // FS segment prefix
            // Handle FS segment override
            cpu->ip++;
            break;

        case 0x65: // GS segment prefix
            // Handle GS segment override
            cpu->ip++;
            break;

        case 0x6C: // INSB
            // Input byte from port DX into ES:DI
            cpu->ip++;
            break;

        case 0x72: // JB/JNAE/JC rel8 (Jump if below/not above or equal/carry)
            offset = (int8_t)cpu_read_byte(cpu, cpu->ip + 1);
            if (cpu->flags & 1) { // If carry flag is set
                cpu->ip += offset + 2;
            } else {
                cpu->ip += 2;
            }
            break;

        case 0x7C: // JL/JNGE rel8 (Jump if less/not greater or equal)
            offset = (int8_t)cpu_read_byte(cpu, cpu->ip + 1);
            if ((cpu->flags & 0x80) != 0) { // If sign flag is set
                cpu->ip += offset + 2;
            } else {
                cpu->ip += 2;
            }
            break;

        case 0x8E: // MOV Sreg, r/m16
            modrm = cpu_read_byte(cpu, cpu->ip + 1);
            reg1 = modrm & 0x07;
            switch((modrm >> 3) & 0x07) {
                case 0: cpu->es = cpu->registers[reg1] & 0xFFFF; break;
                case 1: cpu->cs = cpu->registers[reg1] & 0xFFFF; break;
                case 2: cpu->ss = cpu->registers[reg1] & 0xFFFF; break;
                case 3: cpu->ds = cpu->registers[reg1] & 0xFFFF; break;
                case 4: cpu->fs = cpu->registers[reg1] & 0xFFFF; break;
                case 5: cpu->gs = cpu->registers[reg1] & 0xFFFF; break;
            }
            cpu->ip += 2;
            break;

        case 0xB4: // MOV AH, imm8
            value = cpu_read_byte(cpu, cpu->ip + 1);
            cpu->registers[0] = (cpu->registers[0] & 0xFFFF00FF) | (value << 8);
            cpu->ip += 2;
            break;

        case 0xB7: // MOV BH, imm8
            value = cpu_read_byte(cpu, cpu->ip + 1);
            cpu->registers[3] = (cpu->registers[3] & 0xFFFF00FF) | (value << 8);
            cpu->ip += 2;
            break;

        case 0xBC: // MOV SP, imm16
            value = cpu_read_byte(cpu, cpu->ip + 1) |
                   (cpu_read_byte(cpu, cpu->ip + 2) << 8);
            cpu->registers[7] = (cpu->registers[7] & 0xFFFF0000) | value;
            cpu->ip += 3;
            break;

        case 0xBE: // MOV SI, imm16
            value = cpu_read_byte(cpu, cpu->ip + 1) |
                   (cpu_read_byte(cpu, cpu->ip + 2) << 8);
            cpu->registers[4] = (cpu->registers[4] & 0xFFFF0000) | value;
            cpu->ip += 3;
            break;

        case 0xCD: // INT imm8 - Software interrupt
            value = cpu_read_byte(cpu, cpu->ip + 1);
            // Handle interrupt (for bootloader, mainly INT 10h for video)
            if (value == 0x10) {
                // Handle video interrupt
                if ((cpu->registers[0] >> 8) == 0x0E) {
                    // Teletype output
                    printf("%c", cpu->registers[0] & 0xFF);
                }
            }
            cpu->ip += 2;
            break;

        case 0xD8: // FXXX - Floating point instruction
            // For bootloader, we can ignore FPU instructions
            cpu->ip += 2;
            break;

        case 0xE8: // CALL rel16
            offset = (int16_t)(cpu_read_byte(cpu, cpu->ip + 1) |
                              (cpu_read_byte(cpu, cpu->ip + 2) << 8));
            cpu->registers[7] -= 2;
            cpu_write_word(cpu, cpu->registers[7], cpu->ip + 3);
            cpu->ip += offset + 3;
            break;

// Add these cases to the switch statement:

        case 0x0A: // OR AL, r/m8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            if (reg1 < 8) {
                cpu->registers[0] = (cpu->registers[0] & 0xFFFFFF00) |
                    ((cpu->registers[0] | cpu->registers[reg1]) & 0xFF);
            }
            cpu->ip += 2;
            break;

        case 0x14: // ADC AL, imm8
            value = cpu_read_byte(cpu, cpu->ip + 1);
            uint32_t carry = (cpu->flags & 2) ? 1 : 0;
            uint32_t result = (cpu->registers[0] & 0xFF) + value + carry;
            cpu->registers[0] = (cpu->registers[0] & 0xFFFFFF00) | (result & 0xFF);
            cpu->flags = (result & 0xFF) ? 0 : 1;  // Set ZF
            cpu->flags |= (result > 0xFF) ? 2 : 0;  // Set CF
            cpu->ip += 2;
            break;

        case 0x16: // PUSH SS
            cpu->registers[7] -= 2;
            cpu_write_word(cpu, cpu->registers[7], cpu->ss);
            cpu->ip++;
            break;

        case 0x18: // SBB r/m8, r8
            reg1 = cpu_read_byte(cpu, cpu->ip + 1);
            reg2 = cpu_read_byte(cpu, cpu->ip + 2);
            if (reg1 < 8 && reg2 < 8) {
                carry = (cpu->flags & 2) ? 1 : 0;
                result = cpu->registers[reg1] - cpu->registers[reg2] - carry;
                cpu->registers[reg1] = result;
                cpu->flags = (result == 0) ? 1 : 0;  // ZF
                cpu->flags |= (result < 0) ? 2 : 0;  // CF
            }
            cpu->ip += 3;
            break;

        case 0x4B: // DEC BX
            cpu->registers[3] = (cpu->registers[3] & 0xFFFF0000) |
                ((cpu->registers[3] - 1) & 0xFFFF);
            cpu->flags = ((cpu->registers[3] & 0xFFFF) == 0) ? 1 : 0;
            cpu->ip++;
            break;

        case 0x4D: // DEC BP
            cpu->registers[5] = (cpu->registers[5] & 0xFFFF0000) |
                ((cpu->registers[5] - 1) & 0xFFFF);
            cpu->flags = ((cpu->registers[5] & 0xFFFF) == 0) ? 1 : 0;
            cpu->ip++;
            break;

        case 0x58: // POP AX
            cpu->registers[0] = (cpu->registers[0] & 0xFFFF0000) |
                cpu_read_word(cpu, cpu->registers[7]);
            cpu->registers[7] += 2;
            cpu->ip++;
            break;

        default:
            printf("Unknown opcode: 0x%02X at IP: 0x%08X\n", opcode, cpu->ip);
            cpu->ip++;
    }
}

void cpu_load_program(CPU* cpu, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f) {
        fread(cpu->memory, 1, MEMORY_SIZE, f);
        fclose(f);
    } else {
        printf("Failed to load program: %s\n", filename);
    }
}