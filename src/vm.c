#include <vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ISO_SECTOR_SIZE 2048
#define FLOPPY_SECTOR_SIZE 512

// Read ISO file into memory
static int load_iso(VM* vm, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open ISO file: %s\n", filename);
        return 0;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Load first sector (boot sector) at 0x7C00
    uint8_t sector[ISO_SECTOR_SIZE];
    if (fread(sector, 1, ISO_SECTOR_SIZE, file) != ISO_SECTOR_SIZE) {
        printf("Failed to read boot sector\n");
        fclose(file);
        return 0;
    }

    // Copy boot sector to memory at 0x7C00 (standard boot location)
    memcpy(&vm->cpu.memory[0x7C00], sector, FLOPPY_SECTOR_SIZE);

    // Set initial CPU state for booting
    vm->cpu.ip = 0x7C00;             // Start execution at boot sector
    vm->cpu.registers[7] = 0x7C00;   // Stack starts at boot sector
    vm->cpu.cs = 0;                  // Code segment
    vm->cpu.ds = 0;                  // Data segment
    vm->cpu.es = 0;                  // Extra segment
    vm->cpu.ss = 0;                  // Stack segment

    // Store ISO information for later disk operations
    vm->disk_file = file;
    vm->disk_size = size;
    
    return 1;
}

int vm_init(VM* vm) {
    // Initialize CPU and VGA
    cpu_init(&vm->cpu);
    if (!vga_init(&vm->vga)) {
        return 0;
    }

    // Initialize disk state
    vm->disk_file = NULL;
    vm->disk_size = 0;

    // Create memory write hook table
    vm->write_hooks = NULL;
    vm->num_write_hooks = 0;

    // Add VGA memory hook
    vm_add_write_hook(vm, VGA_MEMORY_START, VGA_MEMORY_SIZE, 
        (WriteHookFn)vga_write_memory, &vm->vga);

    return 1;
}

// Memory write hook system
void vm_add_write_hook(VM* vm, uint32_t start, uint32_t size, 
                      WriteHookFn hook, void* data) {
    vm->num_write_hooks++;
    vm->write_hooks = realloc(vm->write_hooks, 
        vm->num_write_hooks * sizeof(WriteHook));
    
    WriteHook* new_hook = &vm->write_hooks[vm->num_write_hooks - 1];
    new_hook->start = start;
    new_hook->end = start + size;
    new_hook->hook = hook;
    new_hook->data = data;
}

// Handle memory writes with hooks
void vm_write_memory(VM* vm, uint32_t addr, uint8_t value) {
    // First write to actual memory
    if (addr < MEMORY_SIZE) {
        vm->cpu.memory[addr] = value;
    }

    // Then check hooks
    for (int i = 0; i < vm->num_write_hooks; i++) {
        WriteHook* hook = &vm->write_hooks[i];
        if (addr >= hook->start && addr < hook->end) {
            hook->hook(hook->data, addr, value);
        }
    }
}

void vm_handle_int10(VM* vm) {
    uint8_t ah = (vm->cpu.registers[0] >> 8) & 0xFF;
    uint8_t al = vm->cpu.registers[0] & 0xFF;
    
    switch (ah) {
        case 0x0E:  // Teletype output
            {
                uint32_t offset = ((vm->vga.cursor_y * VGA_WIDTH) + vm->vga.cursor_x) * 2;
                vm_write_memory(vm, VGA_MEMORY_START + offset, al);
                vm_write_memory(vm, VGA_MEMORY_START + offset + 1, 0x07); // Light gray on black
                
                vm->vga.cursor_x++;
                if (vm->vga.cursor_x >= VGA_WIDTH) {
                    vm->vga.cursor_x = 0;
                    vm->vga.cursor_y++;
                    if (vm->vga.cursor_y >= VGA_HEIGHT) {
                        vm->vga.cursor_y = VGA_HEIGHT - 1;
                        // Could add scrolling here if needed
                    }
                }
            }
            break;
    }
}

void vm_run(VM* vm) {
    SDL_Event event;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Emulate several cycles per frame
        for (int i = 0; i < 1000 && running; i++) {
            // Safely check for interrupts
            if (vm->cpu.ip < MEMORY_SIZE - 1) {  // Ensure we can read two bytes
                uint8_t opcode = vm->cpu.memory[vm->cpu.ip];
                if (opcode == 0xCD && vm->cpu.ip < MEMORY_SIZE - 2) {
                    uint8_t int_num = vm->cpu.memory[vm->cpu.ip + 1];
                    switch (int_num) {
                        case 0x10:
                            vm_handle_int10(vm);
                            break;
                        case 0x13:
                            vm_handle_disk_interrupt(vm);
                            break;
                    }
                }
            }

            // Execute instruction
            cpu_emulate_cycle(&vm->cpu);

            // Safely check VGA memory writes
            if (vm->cpu.last_write_addr != 0xFFFFFFFF && 
                vm->cpu.last_write_addr >= VGA_MEMORY_START && 
                vm->cpu.last_write_addr < VGA_MEMORY_START + VGA_MEMORY_SIZE) {
                // Only write if the address is valid
                if (vm->cpu.last_write_addr < MEMORY_SIZE) {
                    vm_write_memory(vm, 
                                  vm->cpu.last_write_addr, 
                                  vm->cpu.memory[vm->cpu.last_write_addr]);
                }
            }

            // Check if IP is still valid
            if (vm->cpu.ip >= MEMORY_SIZE) {
                printf("CPU IP out of bounds: 0x%08X\n", vm->cpu.ip);
                running = 0;
                break;
            }
        }

        if (running) {
            vga_update(&vm->vga);
            SDL_Delay(16);
        }
    }
}

void vm_cleanup(VM* vm) {
    if (vm->disk_file) {
        fclose(vm->disk_file);
    }
    if (vm->write_hooks) {
        free(vm->write_hooks);
    }
    vga_cleanup(&vm->vga);
}

// Function to load and run an ISO
int vm_load_iso(VM* vm, const char* filename) {
    return load_iso(vm, filename);
}

// INT 13h handler for disk operations
void vm_handle_disk_interrupt(VM* vm) {
    uint8_t function = (vm->cpu.registers[0] >> 8) & 0xFF;
    uint8_t cylinder = (vm->cpu.registers[1] >> 8) & 0xFF;
    uint8_t sector = vm->cpu.registers[1] & 0xFF;
    uint8_t head = (vm->cpu.registers[2] >> 8) & 0xFF;
    uint8_t count = (vm->cpu.registers[0] & 0xFF);
    uint16_t buffer_seg = vm->cpu.es;
    uint16_t buffer_off = vm->cpu.registers[3] & 0xFFFF;

    switch (function) {
        case 0x02: // Read sectors
            if (vm->disk_file) {
                long offset = ((cylinder * 2 * 18) + (head * 18) + (sector - 1)) * 
                            FLOPPY_SECTOR_SIZE;
                fseek(vm->disk_file, offset, SEEK_SET);
                
                uint32_t buffer_addr = (buffer_seg << 4) + buffer_off;
                uint8_t buffer[FLOPPY_SECTOR_SIZE];
                
                for (int i = 0; i < count; i++) {
                    if (fread(buffer, 1, FLOPPY_SECTOR_SIZE, vm->disk_file) == FLOPPY_SECTOR_SIZE) {
                        for (int j = 0; j < FLOPPY_SECTOR_SIZE; j++) {
                            vm_write_memory(vm, buffer_addr + j, buffer[j]);
                        }
                        buffer_addr += FLOPPY_SECTOR_SIZE;
                    }
                }
                
                // Clear CF to indicate success
                vm->cpu.flags &= ~1;
            }
            break;
    }
}