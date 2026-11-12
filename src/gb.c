// init gameboy to a state right after the boot rom has finished and is safe to load a rom
// this assumes gb is zero inited or inited from a previous invocation
void gb_init(GameBoy *gb, SerialCallback serial_io) {
    gb_free(gb);
    memset(gb, 0, sizeof(*gb));
    gb->af = 0x01b0;
    gb->c = 0x13;
    gb->e = 0xd8;
    gb->h = 0x01;
    gb->l = 0x4d;
    gb->pc = 0x0100;
    gb->sp = 0xFFFE;
    gb->serial_io = serial_io;
}

int gb_load_rom(GameBoy *gb, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    // todo: round up to next bank size and then next power of 2
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0x8000) n = 0x8000;

    free(gb->rom);
    gb->rom = malloc(n);
    if (!gb->rom) goto close_file;
    gb->rom_len = n;

    size_t nread = fread(gb->rom, 1, n, f);
    if (!nread && ferror(f)) goto free_rom;

    fclose(f);
    return 0;
free_rom:
    free(gb->rom);
    gb->rom = NULL;
close_file:
    fclose(f);
    return 1;
}

void gb_free(GameBoy *gb) {
    if (!gb) return;
    free(gb->rom);
    gb->rom = NULL;
}

static u8 gb_read_rom(GameBoy *gb, u16 addr) {
    return gb->rom[addr & 0x7fff];
}

static u8 gb_read_vram(GameBoy *gb, u16 addr) {
    return gb->vram[addr & 0x1fff];
}

static u8 gb_read_eram(GameBoy *gb, u16 addr) {
    (void)gb, (void)addr;
    return 0xff;
}

static u8 gb_read_wram(GameBoy *gb, u16 addr) {
    return gb->wram[addr & 0x1fff];
}

static u8 gb_read_high(GameBoy *gb, u16 addr) {
    if (addr < 0xff80) {
        return gb->io_regs[addr & 0x7f];
    } else if (addr < 0xffff) {
        return gb->hram[addr & 0x7f];
    } else {
        return gb->interrupt_enable;
    }
}

typedef u8 (*MemReadFunc)(GameBoy *, u16);
static MemReadFunc mem_read_map[16] = {// 0x0000 - 0x7fff. Assuming no mbc for now
                                       gb_read_rom, gb_read_rom, gb_read_rom, gb_read_rom,
                                       gb_read_rom, gb_read_rom, gb_read_rom, gb_read_rom,
                                       // 0x8000 - 0x9fff
                                       gb_read_vram, gb_read_vram,
                                       // 0xa000 - 0xbfff. Skipping for now
                                       gb_read_eram, gb_read_eram,
                                       // 0xc000 - 0xdfff
                                       gb_read_wram, gb_read_wram,
                                       // 0xe000 - 0xefff
                                       gb_read_wram,
                                       // 0xf000 - 0xffff
                                       gb_read_high};

u8 gb_mem_read(GameBoy *gb, u16 addr) {
    return mem_read_map[addr >> 12](gb, addr);
}

static void gb_write_rom(GameBoy *gb, u16 addr, u8 val) {
    (void)gb, (void)addr, (void)val;
}

static void gb_write_vram(GameBoy *gb, u16 addr, u8 val) {
    gb->vram[addr & 0x1fff] = val;
}

static void gb_write_eram(GameBoy *gb, u16 addr, u8 val) {
    (void)gb, (void)addr, (void)val;
}

static void gb_write_wram(GameBoy *gb, u16 addr, u8 val) {
    gb->wram[addr & 0x1fff] = val;
}

static void gb_write_high(GameBoy *gb, u16 addr, u8 val) {
    if (addr < 0xff80) {
        gb_io_write(gb, addr, val);
    } else if (addr < 0xffff) {
        gb->hram[addr & 0x7f] = val;
    } else {
        gb->interrupt_enable = val;
    }
}

typedef void (*MemWriteFunc)(GameBoy *gb, u16, u8);
static MemWriteFunc mem_write_map[16] = {// 0x0000 - 0x7fff. Assuming no mbc for now
                                         gb_write_rom, gb_write_rom, gb_write_rom, gb_write_rom,
                                         gb_write_rom, gb_write_rom, gb_write_rom, gb_write_rom,
                                         // 0x8000 - 0x9fff
                                         gb_write_vram, gb_write_vram,
                                         // 0xa000 - 0xbfff. Skipping for now
                                         gb_write_eram, gb_write_eram,
                                         // 0xc000 - 0xdfff
                                         gb_write_wram, gb_write_wram,
                                         // 0xe000 - 0xefff
                                         gb_write_wram,
                                         // 0xf000 - 0xffff
                                         gb_write_high};

void gb_mem_write(GameBoy *gb, u16 addr, u8 val) {
    mem_write_map[addr >> 12](gb, addr, val);
}

#define GB_IO_SB   0x01
#define GB_IO_SC   0x02
#define GB_IO_DIV  0x04
#define GB_IO_IF   0x0f

void gb_io_write(GameBoy *gb, u16 addr, u8 val) {
    switch (addr & 0x7f) {
    case GB_IO_SB: gb->io_regs[GB_IO_SB] = val; break;
    case GB_IO_SC:
        val |= 2;
        gb->io_regs[GB_IO_SC] = val | (~0x83);
        if (((val & 0x80) && (val & 0x01)) && gb->serial_io) {
            gb->serial_io(gb->io_regs[GB_IO_SB]);
        }
        break;
    case GB_IO_DIV: gb->io_regs[GB_IO_DIV] = 0x00; break;
    case GB_IO_IF: gb->io_regs[GB_IO_IF] = val | 0xe0 break;
    }
}