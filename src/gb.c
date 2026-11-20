#define GB_IO_SB   0x01
#define GB_IO_SC   0x02
#define GB_IO_DIV  0x04
#define GB_IO_TIMA 0x05
#define GB_IO_TMA  0x06
#define GB_IO_TAC  0x07
#define GB_IO_IF   0x0f

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
    gb->io_regs[GB_IO_TAC] = 0xf8;
    gb->io_regs[GB_IO_IF] = 0xe1;
    gb->div_counter = 0xabcc;
    gb->serial_io = serial_io;
    gb->rom_bank = 1;
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

internal u8 gb_read_rom(GameBoy *gb, u16 addr) {
    return gb->rom[addr & 0x7fff];
}

internal u8 gb_read_vram(GameBoy *gb, u16 addr) {
    return gb->vram[addr & 0x1fff];
}

internal u8 gb_read_eram(GameBoy *gb, u16 addr) {
    return gb->eram[addr & 0x1fff];
}

internal u8 gb_read_wram(GameBoy *gb, u16 addr) {
    return gb->wram[addr & 0x1fff];
}

internal u8 gb_read_io(GameBoy *gb, u16 addr) {
    u8 reg = addr & 0x7f;
    if (reg == GB_IO_DIV)
        return gb->div_counter >> 8;
    else
        return gb->io_regs[reg];
}

internal u8 gb_read_high(GameBoy *gb, u16 addr) {
    if (addr < 0xff80) {
        return gb_read_io(gb, addr);
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

internal void gb_mbc_write_rom(GameBoy *gb, u16 addr, u8 val) {
    switch (gb->mbc_ty) {
    case MBC_1: {
        switch (addr >> 13) {
        case 0: gb->ram_enable = (val & 0xf) == 0xa; break; // 0000-1fff
        case 1: gb->mbc1.rom_bank = val; break;             // 2000-3fff
        case 2: gb->mbc1.ram_bank = val; break;             // 4000-5fff
        case 3: gb->mbc1.mode = val; break;                 // 6000-7fff
        }

        // todo: banking mode
        // when it is 0 or 1 - the 2 ram_bank bits apply
        //     - 0: 0000-3fff is bank 0 and a000-bfff is locked at 0
        //     - 1: allow for bank switching.
        gb->rom_bank = ((gb->mbc1.ram_bank) << 5) | (gb->mbc1.rom_bank);
        if ((gb->rom_bank & 0x1f) == 0) {
            gb->rom_bank += 1;
        }
    }
    }
}

internal void gb_write_vram(GameBoy *gb, u16 addr, u8 val) {
    gb->vram[addr & 0x1fff] = val;
}

internal void gb_write_eram(GameBoy *gb, u16 addr, u8 val) {
    gb->eram[addr & 0x1fff] = val;
}

internal void gb_write_wram(GameBoy *gb, u16 addr, u8 val) {
    gb->wram[addr & 0x1fff] = val;
}

internal void gb_write_io(GameBoy *gb, u16 addr, u8 val) {
    switch (addr & 0x7f) {
    case GB_IO_SB: gb->io_regs[GB_IO_SB] = val; break;
    case GB_IO_SC:
        val |= 2;
        gb->io_regs[GB_IO_SC] = val | (~0x83);
        if (((val & 0x80) && (val & 0x01)) && gb->serial_io) {
            gb->serial_io(gb->io_regs[GB_IO_SB]);
        }
        break;
    case GB_IO_DIV: gb->div_counter = 0; break;
    case GB_IO_TMA: gb->io_regs[GB_IO_TMA] = val; break;
    case GB_IO_TIMA: gb->io_regs[GB_IO_TIMA] = val; break;
    case GB_IO_TAC: gb->io_regs[GB_IO_TAC] = val | 0xf8; break;
    case GB_IO_IF: gb->io_regs[GB_IO_IF] = val | 0xe0; break;
    }
}

internal void gb_write_high(GameBoy *gb, u16 addr, u8 val) {
    if (addr < 0xff80) {
        gb_write_io(gb, addr, val);
    } else if (addr < 0xffff) {
        gb->hram[addr & 0x7f] = val;
    } else {
        gb->interrupt_enable = val;
    }
}

typedef void (*MemWriteFunc)(GameBoy *gb, u16, u8);
static MemWriteFunc mem_write_map[16] = {// 0x0000 - 0x7fff
                                         gb_mbc_write_rom, gb_mbc_write_rom, gb_mbc_write_rom,
                                         gb_mbc_write_rom, gb_mbc_write_rom, gb_mbc_write_rom,
                                         gb_mbc_write_rom, gb_mbc_write_rom,
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

#define GB_INT_VBLANK   (1 << 0)
#define GB_INT_LCD      (1 << 1)
#define GB_INT_TIMER    (1 << 2)
#define GB_INT_SERIAL   (1 << 3)
#define GB_INT_JOYPAD   (1 << 4)

void gb_timer_tick(GameBoy *gb, int cycles) {
    static u16 falling_bit[] = {1 << 9, 1 << 3, 1 << 5, 1 << 7};
    while (cycles--) {
        u16 old = gb->div_counter;
        gb->div_counter += 1;

        u8 tac = gb->io_regs[GB_IO_TAC];
        if (!(tac & 0x04)) continue;

        u16 bit = falling_bit[tac & 0x03];
        if ((old & bit) && !(gb->div_counter & bit)) {
            gb->io_regs[GB_IO_TIMA]++;
            if (gb->io_regs[GB_IO_TIMA] == 0) {
                gb->io_regs[GB_IO_TIMA] = gb->io_regs[GB_IO_TMA];
                gb->io_regs[GB_IO_IF] |= GB_INT_TIMER;
            }
        }
    }
}

void gb_ppu_tick(GameBoy *gb, int cycles) {
    // halt bug test needs vblank interrupt working...
    // The PPU writes one dot (pixel) in 1 t-cycle.
    // One line is 456 dots and we need to draw 144 lines before entering vblank.
    // So after 456 * 144 dots, we can enable the vblank interrupt.
    while (cycles--) {
        gb->ppu_ticks = (gb->ppu_ticks + 1) % TCYCLES_PER_FRAME;
        if (gb->ppu_ticks == 456 * 144) {
            gb->io_regs[GB_IO_IF] |= GB_INT_VBLANK;
        }
    }
}