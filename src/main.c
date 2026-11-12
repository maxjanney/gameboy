#include "base.h"
#include "gb.h"

#include "gb.c"
#include "cpu.c"

#define OUTPUT_SIZE 1024

char serial_buf[OUTPUT_SIZE];
size_t pos = 0;

GameBoy gb;

void serial(u8 val) {
	if (pos < OUTPUT_SIZE - 1) {
		serial_buf[pos++] = val;
		serial_buf[pos] = '\0';
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Give a path to the rom to run\n");
		return 1;
	}

	gb_init(&gb, serial);
	if (gb_load_rom(&gb, argv[1])) {
		fprintf(stderr, "Failed to load rom\n");
		return 1;
	}

	u16 prev_pc = 0;
	while (prev_pc != gb.pc || gb.halted) {
		prev_pc = gb.pc;
		gb_cpu_step(&gb);
	}

	printf("%s", serial_buf);
	gb_free(&gb);
}
