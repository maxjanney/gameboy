void gb_init(GameBoy *gb) {
    gb->af = 0x01b0;
    gb->c = 0x13;
    gb->e = 0xd8;
    gb->h = 0x01;
    gb->l = 0x4d;
    gb->pc = 0x0100;
    gb->sp = 0xFFFE;
}

int gb_load_rom(GameBoy *gb, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 1;
    
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (gb->rom) free(gb->rom);
    gb->rom = malloc(n);
    if (!gb->rom) goto close_file;

    size_t nread = fread(gb->rom, 1, n, f);
    if (!nread && ferror(f)) goto free_rom;
    
    return 0;
free_rom:
    free(gb->rom);
close_file:
    fclose(f);
    return 1;
}

void gb_cpu_step(GameBoy *gb) {}

void gb_free(GameBoy *gb) {
    if (!gb) return;
    if (gb->rom) free(gb->rom);
}

void gb_mem_write(GameBoy *gb, u16 addr, u8 val) {

}

void gb_io_write(GameBoy *gb, u16 addr, u8 val) {
    u8 io_reg = addr & 0x7f;
    switch (io_reg) {
    case GB_IO_SB:
        gb->io_regs[io_reg] = val;
        break;
    case GB_IO_SC:
        val |= 2;
        gb->io_regs[io_reg] = val | (~0x83);
        if (((val & 0x80) && (val & 0x01)) && gb->serial_io) {
            gb->serial_io(gb, gb->io_regs[GB_IO_SB]);
        }
        break;
    }
}