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

#define todo_opcode() {                                               \
    fprintf(stderr, "Unimplemented opcode encountered: 0x%02X\n", opcode);   \
    abort();                                                            \
}                                                              

#define ld_r_r(dst, src) {  \
    gb->dst = gb->src;      \
    return 1;               \
}

#define ld_r_hl(r) {                    \
    gb->r = gb_mem_read(gb, gb->hl);    \
    return 2;                           \
}

int gb_cpu_step(GameBoy *gb) {
    u8 opcode = gb_mem_read(gb, gb->pc++);
    switch (opcode) {
    case 0x40: ld_r_r(b, b)
    case 0x41: ld_r_r(b, c)
    case 0x42: ld_r_r(b, d)
    case 0x43: ld_r_r(b, e)
    case 0x44: ld_r_r(b, h)
    case 0x45: ld_r_r(b, l)
    case 0x46: ld_r_hl(b)
    case 0x47: ld_r_r(b, a)
    case 0x48: ld_r_r(c, b)
    case 0x49: ld_r_r(c, c)
    case 0x4a: ld_r_r(c, d)
    case 0x4b: ld_r_r(c, e)
    case 0x4c: ld_r_r(c, h)
    case 0x4d: ld_r_r(c, l)
    case 0x4e: ld_r_hl(c)
    case 0x4f: ld_r_r(c, a)
    case 0x50: ld_r_r(d, b)
    case 0x51: ld_r_r(d, c)
    case 0x52: ld_r_r(d, d)
    case 0x53: ld_r_r(d, e)
    case 0x54: ld_r_r(d, h)
    case 0x55: ld_r_r(d, l)
    case 0x56: ld_r_hl(d)
    case 0x57: ld_r_r(d, a)
    case 0x58: ld_r_r(e, b)
    case 0x59: ld_r_r(e, c)
    case 0x5a: ld_r_r(e, d)
    case 0x5b: ld_r_r(e, e)
    case 0x5c: ld_r_r(e, h)
    case 0x5d: ld_r_r(e, l)
    case 0x5e: ld_r_hl(e)
    case 0x5f: ld_r_r(e, a)
    case 0x60: ld_r_r(h, b)
    case 0x61: ld_r_r(h, c)
    case 0x62: ld_r_r(h, d)
    case 0x63: ld_r_r(h, e)
    case 0x64: ld_r_r(h, h)
    case 0x65: ld_r_r(h, l)
    case 0x66: ld_r_hl(h)
    case 0x67: ld_r_r(h, a)
    case 0x68: ld_r_r(l, b)
    case 0x69: ld_r_r(l, c)
    case 0x6a: ld_r_r(l, d)
    case 0x6b: ld_r_r(l, e)
    case 0x6c: ld_r_r(l, h)
    case 0x6d: ld_r_r(l, l)
    case 0x6e: ld_r_hl(l)
    case 0x6f: ld_r_r(l, a)
    case 0x70: todo_opcode()
    case 0x71: todo_opcode()
    case 0x72: todo_opcode()
    case 0x73: todo_opcode()
    case 0x74: todo_opcode()
    case 0x75: todo_opcode()
    case 0x76: todo_opcode()// halt
    case 0x77: todo_opcode()
    case 0x78: ld_r_r(a, b)
    case 0x79: ld_r_r(a, c)
    case 0x7a: ld_r_r(a, d)
    case 0x7b: ld_r_r(a, e)
    case 0x7c: ld_r_r(a, h)
    case 0x7d: ld_r_r(a, l)
    case 0x7e: todo_opcode()
    case 0x7f: ld_r_r(a, a)
    default: todo_opcode()
    }
}

void gb_free(GameBoy *gb) {
    if (!gb) return;
    if (gb->rom) free(gb->rom);
}

void gb_mem_write(GameBoy *gb, u16 addr, u8 val) {

}

u8 gb_mem_read(GameBoy *gb, u16 addr) {
    return 0;
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