#include "base/base.h"
#include "gb/gb.h"

#include "base/base.c"
#include "gb/gb.c"

void serial(GameBoy *gb, u8 val) {
	putchar(val);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Give a path to the rom to run\n");
		return 1;
	}

	GameBoy gb = (GameBoy){ .serial_io = serial };
	gb_init(&gb);
	if (gb_load_rom(&gb, argv[1])) {
		fprintf(stderr, "Failed to load rom\n");
		return 1;
	}

	gb_free(&gb);
}
