#include "base.h"
#include "gb.h"

#include "gb.c"
#include "cpu.c"

#define OUTPUT_SIZE 1024
#define TCYCLES_PER_FRAME 70224

typedef enum TestType TestType;
enum TestType {
    // blargg test outputs to Ram
    Ram,
    // blargg test outputs via serial port
    Serial,
};

char serial_buf[OUTPUT_SIZE];
size_t pos = 0;

GameBoy gb;

void serial(u8 val) {
    if (pos < OUTPUT_SIZE - 1) {
        serial_buf[pos++] = val;
        serial_buf[pos] = '\0';
    }
}

void run_serial_test() {
    u16 prev_pc = 0;
    while (prev_pc != gb.pc || gb.halted) {
        prev_pc = gb.pc;
        int cycles = gb_cpu_step(&gb);
        gb_timer_tick(&gb, cycles);
    }

    printf("%s", serial_buf);
}

void run_ram_test() {
    // Output and final result are written to 0xa000.
    // To determine if the result at 0xa000 is a result and not random,
    // 0xa001-0xa003 are written with a signature: 0xde, 0xb0, and 0x61.
    // If these are present, then the text string and final result status are valid.
    //
    // 0xa000 holds the overall status (0x80 means its running, anything else holds the final
    // result).
    //
    // All text output is stored in a zero-terminated string at 0xa004.
    //
    // This will run the cpu, then the timer for about 1 frame and check
    // if 0xa001==0xde and 0xa002==0xb0 and 0xa003==0x61 and 0xa000!=0x80:
    //     print("done"); printf("%s\n", 0xa004);
    // else
    //     print("not done...")
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Give a path to the rom to run\n");
        return 1;
    }

    TestType test_ty = strstr(argv[1], "halt_bug") == NULL;

    gb_init(&gb, serial);
    if (gb_load_rom(&gb, argv[1])) {
        fprintf(stderr, "Failed to load rom\n");
        return 1;
    }

    if (test_ty == Serial) {
        run_serial_test();
    } else {
        run_ram_test();
    }

    gb_free(&gb);
}
