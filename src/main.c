#include <vm.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    VM vm;
    if (!vm_init(&vm)) {
        printf("Failed to initialize VM\n");
        return 1;
    }

    cpu_load_program(&vm.cpu, argv[1]);
    vm_run(&vm);
    vm_cleanup(&vm);

    return 0;
}