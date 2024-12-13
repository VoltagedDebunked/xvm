#ifndef VM_H
#define VM_H

#include <cpu.h>
#include <vga.h>
#include <stdint.h>
#include <stdio.h>

// Function pointer type for memory write hooks
typedef void (*WriteHookFn)(void* data, uint32_t addr, uint8_t value);

// Structure for memory write hooks
typedef struct {
    uint32_t start;
    uint32_t end;
    WriteHookFn hook;
    void* data;
} WriteHook;

// VM structure
typedef struct {
    CPU cpu;
    VGA vga;
    FILE* disk_file;
    long disk_size;
    WriteHook* write_hooks;
    int num_write_hooks;
} VM;

int vm_init(VM* vm);
void vm_run(VM* vm);
void vm_cleanup(VM* vm);
int vm_load_iso(VM* vm, const char* filename);
void vm_add_write_hook(VM* vm, uint32_t start, uint32_t size, WriteHookFn hook, void* data);
void vm_write_memory(VM* vm, uint32_t addr, uint8_t value);
void vm_handle_disk_interrupt(VM* vm);
void vm_handle_int10(VM* vm);

#endif // VM_H