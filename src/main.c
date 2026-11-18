#include "base.h"
#include "gb.h"

#include "gb.c"
#include "cpu.c"

#define OUTPUT_SIZE 1024
#define FRAME_TIMEOUT 120

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
    int overshoot = 0;
    int success = 0;
    int frame;
    for (frame = 0; frame < FRAME_TIMEOUT; frame++) {
        int cycles = TCYCLES_PER_FRAME + overshoot;
        while (cycles > 0) {
            int n = gb_cpu_step(&gb);
            gb_timer_tick(&gb, n);
            gb_ppu_tick(&gb, n);
            cycles -= n;
        }
        overshoot = cycles;

        u8 status = gb_mem_read(&gb, 0xa000);
        if ((gb_mem_read(&gb, 0xa001) == 0xde) &&
            (gb_mem_read(&gb, 0xa002) == 0xb0) &&
            (gb_mem_read(&gb, 0xa003) == 0x61) &&
            (status != 0x80)) {
            success = status == 0;
            break;
        }
    }

    char *msg = frame == FRAME_TIMEOUT ? "frame timeout reached" : (char *)gb.eram + 4;
    printf("%s:\n%s\n", success ? "Passed" : "Failed", msg);
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
