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

    if (gb->rom) free(gb->rom);
    gb->rom = malloc(n);
    if (!gb->rom) goto close_file;

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

#define GB_FLAG_Z (1 << 7)
#define GB_FLAG_N (1 << 6)
#define GB_FLAG_H (1 << 5)
#define GB_FLAG_C (1 << 4)

#define todo_opcode() {                                                     \
    fprintf(stderr, "Unimplemented opcode encountered: 0x%02X\n", opcode);  \
    abort();                                                                \
}

#define ld_r_u8(r) {                    \
    gb->r = gb_mem_read(gb, gb->pc++);  \
    return 2;                           \
}

#define ld_rr_u16(rr) {\
    u8 lo = gb_mem_read(gb, gb->pc++);  \
    u8 hi = gb_mem_read(gb, gb->pc++);  \
    gb->rr = ((u16)hi << 8) | (u16)lo;  \
    return 3;                           \
}

#define ld_r_r(dst, src) {  \
    gb->dst = gb->src;      \
    return 1;               \
}

#define ld_r_hl(r) {                    \
    gb->r = gb_mem_read(gb, gb->hl);    \
    return 2;                           \
}

#define ld_hl_r(r) {                    \
    gb_mem_write(gb, gb->hl, gb->r);    \
    return 2;                           \
}

// addr must be used exactly once so hl++/hl-- only step once
#define ld_addr_a(addr) {               \
    gb_mem_write(gb, (addr), gb->a);    \
    return 2;                           \
}

#define ld_a_addr(addr) {               \
    gb->a = gb_mem_read(gb, (addr));    \
    return 2;                           \
}

#define inc_rr(rr) {    \
    gb->rr += 1;        \
    return 2;           \
}

#define inc_r(r) {                          \
    u8 h = (gb->r & 0xf) == 0xf;            \
    gb->r += 1;                             \
    gb->f = ((gb->r == 0) << 7) /* Z */ |   \
            /* N = 0 */                     \
            (h << 5) | /* H */              \
            (gb->f & GB_FLAG_C) /* C */;    \
    return 1;                               \
}

#define dec_r(r) {                          \
    u8 h = (gb->r & 0xf) == 0;              \
    gb->r -= 1;                             \
    gb->f = ((gb->r == 0) << 7) /* Z */ |   \
            GB_FLAG_N /* N = 1 */ |         \
            (h << 5) /* H */ |              \
            (gb->f & GB_FLAG_C) /* C */;    \
    return 1;                               \
}

#define dec_rr(rr) {    \
    gb->rr -= 1;        \
    return 2;           \
}

#define add_a_r(r) {                            \
    u16 res = gb->a + gb->r;                    \
    u8 h = (gb->a & 0xf) + (gb->r & 0xf) > 0xf; \
    gb->a = (u8)res;                            \
    gb->f = ((gb->a == 0) << 7) | /* Z */       \
            /* N = 0 */                         \
            (h << 5) | /* H */                  \
            ((res > 0xff) << 4); /* C */        \
    return 1;                                   \
}

#define add_a_hl() {                            \
    u8 val = gb_mem_read(gb, gb->hl);           \
    u16 res = val + gb->a;                      \
    u8 h = (gb->a & 0xf) + (val & 0xf) > 0xf;   \
    gb->a = res;                                \
    gb->f = ((gb->a == 0) << 7) | /* Z */       \
            /* N = 0 */                         \
            (h << 5) | /* H */                  \
            ((res > 0xff) << 4); /* C */        \
    return 2;                                   \
}

#define add_hl_rr(rr) {                                 \
    u8 h = (gb->hl & 0xfff) + (gb->rr & 0xfff) > 0xfff; \
    u32 res = gb->hl + gb->rr;                          \
    u8 c = res > 0xffff;                                \
    gb->hl = res;                                       \
    gb->f = (gb->f & GB_FLAG_Z) /* Z */ |               \
            /* N = 0 */                                 \
            (h << 5) /* H */ |                          \
            (c << 4); /* C */                           \
    return 2;                                           \
}

#define adc_a_r(r) {                                        \
    u8 c = (gb->f & GB_FLAG_C) >> 4;                        \
    u16 res = gb->a + gb->r + c;                            \
    u8 h = (gb->a & 0xf) + (gb->r & 0xf) + (c & 0xf) > 0xf; \
    gb->a = res;                                            \
    gb->f = ((gb->a == 0) << 7) | /* Z */                   \
            /* N = 0 */                                     \
            (h << 5) | /* H */                              \
            ((res > 0xff) << 4); /* C */                    \
    return 1;                                               \
}

#define adc_a_hl() {                                        \
    u8 val = gb_mem_read(gb, gb->hl);                       \
    u8 c = (gb->f & GB_FLAG_C) >> 4;                        \
    u16 res = gb->a + val + c;                              \
    u8 h = (gb->a & 0xf) + (val & 0xf) + (c & 0xf) > 0xf;   \
    gb->a = res;                                            \
    gb->f = ((gb->a == 0) << 7) | /* Z */                   \
            /* N = 0 */                                     \
            (h << 5) | /* H */                              \
            ((res > 0xff) << 4); /* C */                    \
    return 2;                                               \
}

#define sub_a_r(r) {                                        \
    u8 c = gb->r > gb->a;                                   \
    u8 h = (((gb->a & 0xf) - (gb->r & 0xf)) & 0x10) != 0;   \
    gb->a -= gb->r;                                         \
    gb->f = ((gb->a == 0) << 7) | /* Z */                   \
            GB_FLAG_N | /* N = 1 */                         \
            (h << 5) | /* H */                              \
            (c << 4); /* C */                               \
    return 1;                                               \
}

#define sub_a_hl() {                                    \
    u8 val = gb_mem_read(gb, gb->hl);                   \
    u8 c = val > gb->a;                                 \
    u8 h = (((gb->a & 0xf) - (val & 0xf)) & 0x10) != 0; \
    gb->a -= val;                                       \
    gb->f = ((gb->a == 0) << 7) | /* Z */               \
            GB_FLAG_N | /* N = 1 */                     \
            (h << 5) | /* H */                          \
            (c << 4); /* C */                           \
    return 2;                                           \
}

#define and_a_r(r) {                            \
    gb->a &= gb->r;                             \
    gb->f = ((gb->a == 0) << 7) | GB_FLAG_H;    \
    return 1;                                   \
}

#define and_a_hl() {                            \
    gb->a &= gb_mem_read(gb, gb->hl);           \
    gb->f = ((gb->a == 0) << 7) | GB_FLAG_H;    \
    return 2;                                   \
}

#define xor_a_r(r) {            \
    gb->a ^= gb->r;             \
    gb->f = (gb->a == 0) << 7;  \
    return 1;                   \
}

#define xor_a_hl() {                     \
    gb->a ^= gb_mem_read(gb, gb->hl);   \
    gb->f = (gb->a == 0) << 7;          \
    return 2;                           \
}

#define or_a_r(r) {             \
    gb->a |= gb->r;             \
    gb->f = (gb->a == 0) << 7;  \
    return 1;                   \
}

#define or_a_hl() {                     \
    gb->a |= gb_mem_read(gb, gb->hl);   \
    gb->f = (gb->a == 0) << 7;          \
    return 2;                           \
}

#define cp_a_r(r) {                                         \
    u8 c = gb->r > gb->a;                                   \
    u8 h = (((gb->a & 0xf) - (gb->r & 0xf)) & 0x10) != 0;   \
    gb->f = ((gb->a == gb->r) << 7) | /* Z */               \
            GB_FLAG_N | /* N = 1 */                         \
            (h << 5) | /* H */                              \
            (c << 4); /* C */                               \
    return 1;                                               \
}

#define cp_a_hl() {                                     \
    u8 val = gb_mem_read(gb, gb->hl);                   \
    u8 c = val > gb->a;                                 \
    u8 h = (((gb->a & 0xf) - (val & 0xf)) & 0x10) != 0; \
    gb->f = ((gb->a == val) << 7) | /* Z */             \
            GB_FLAG_N | /* N = 1 */                     \
            (h << 5) | /* H */                          \
            (c << 4); /* C */                           \
    return 2;                                           \
}

#define jr_rel(cond) {                          \
    i8 offset = (i8)gb_mem_read(gb, gb->pc++);  \
    if (cond) {                                 \
        gb->pc += offset;                       \
        return 3;                               \
    }                                           \
    return 2;                                   \
}

#define pop_rr(rr) {                    \
    u8 lo = gb_mem_read(gb, gb->sp++);  \
    u8 hi = gb_mem_read(gb, gb->sp++);  \
    gb->rr =((u16)hi << 8) | lo;        \
    return 3;                           \
}

#define push_rr(hi, lo) {               \
    gb_mem_write(gb, --gb->sp, gb->hi); \
    gb_mem_write(gb, --gb->sp, gb->lo); \
    return 4;                           \
}

int gb_cpu_step(GameBoy *gb) {
    u8 opcode = gb_mem_read(gb, gb->pc++);
    switch (opcode) {
    case 0x00: return 1;
    case 0x01: ld_rr_u16(bc);
    case 0x02: ld_addr_a(gb->bc);
    case 0x03: inc_rr(bc);
    case 0x04: inc_r(b);
    case 0x05: dec_r(b);
    case 0x06: ld_r_u8(b);
    case 0x07: {
        u8 hi = gb->a >> 7;
        gb->a = (gb->a << 1) | hi;
        gb->f = hi << 4;
        return 1;
    }
    case 0x08: {
        u8 lo = gb_mem_read(gb, gb->pc++);
        u8 hi = gb_mem_read(gb, gb->pc++);
        u16 addr = ((u16)hi << 8) | (u16)lo;
        gb_mem_write(gb, addr, gb->sp & 0xff);
        gb_mem_write(gb, addr + 1, gb->sp >> 8);
        return 5;
    }
    case 0x09: add_hl_rr(bc);
    case 0x0a: ld_a_addr(gb->bc);
    case 0x0b: dec_rr(bc);
    case 0x0c: inc_r(c);
    case 0x0d: dec_r(c);
    case 0x0e: ld_r_u8(c);
    case 0x0f: {
        u8 c = gb->a & 0x01;
        gb->a = (gb->a >> 1) | (c << 7);
        gb->f = c << 4;
        return 1;
    }

    case 0x10: todo_opcode();
    case 0x11: ld_rr_u16(de);
    case 0x12: ld_addr_a(gb->de);
    case 0x13: inc_rr(de);
    case 0x14: inc_r(d);
    case 0x15: dec_r(d);
    case 0x16: ld_r_u8(d);
    case 0x17: {
        u8 c = (gb->f & GB_FLAG_C) >> 4;
        gb->f = (gb->a & 0x80) >> 3;
        gb->a = (gb->a << 1) | c;
        return 1;
    }
    case 0x18: jr_rel(1);
    case 0x19: add_hl_rr(de);
    case 0x1a: ld_a_addr(gb->de);
    case 0x1b: dec_rr(de);
    case 0x1c: inc_r(e);
    case 0x1d: dec_r(e);
    case 0x1e: ld_r_u8(e);
    case 0x1f: {
        u8 c = gb->a & 0x01;
        gb->a >>= 1;
        gb->f = c << 4;
        return 1;
    }

    case 0x20: jr_rel(!(gb->f & GB_FLAG_Z));
    case 0x21: ld_rr_u16(hl);
    case 0x22: ld_addr_a(gb->hl++);
    case 0x23: inc_rr(hl);
    case 0x24: inc_r(h);
    case 0x25: dec_r(h);
    case 0x26: ld_r_u8(h);
    case 0x27: todo_opcode();
    case 0x28: jr_rel(gb->f & GB_FLAG_Z);
    case 0x29: add_hl_rr(hl);
    case 0x2a: ld_a_addr(gb->hl++);
    case 0x2b: dec_rr(hl);
    case 0x2c: inc_r(l);
    case 0x2d: dec_r(l);
    case 0x2e: ld_r_u8(l);
    case 0x2f: todo_opcode();

    case 0x30: jr_rel(!(gb->f & GB_FLAG_C));
    case 0x31: ld_rr_u16(sp);
    case 0x32: ld_addr_a(gb->hl--);
    case 0x33: inc_rr(sp);
    case 0x34: {
        u8 res = gb_mem_read(gb, gb->hl) + 1;
        gb_mem_write(gb, gb->hl, res);
        gb->f = ((res == 0) << 7) /* Z */ |
                /* N = 0 */
                (((res & 0xf) == 0) << 5) | /* H */
                (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x35: {
        u8 val = gb_mem_read(gb, gb->hl) - 1;
        gb_mem_write(gb, gb->hl, val);
        gb->f = (val == 0 ? GB_FLAG_Z : 0) |
                GB_FLAG_N |
                ((val & 0xf) == 0xf ? GB_FLAG_H : 0) |
                (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x36: gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->pc++)); return 3;
    case 0x37: todo_opcode();
    case 0x38: jr_rel(gb->f & GB_FLAG_C);
    case 0x39: add_hl_rr(sp);
    case 0x3a: ld_a_addr(gb->hl--);
    case 0x3b: dec_rr(sp);
    case 0x3c: inc_r(a);
    case 0x3d: dec_r(a);
    case 0x3e: ld_r_u8(a);
    case 0x3f: todo_opcode();

    case 0x40: ld_r_r(b, b);
    case 0x41: ld_r_r(b, c);
    case 0x42: ld_r_r(b, d);
    case 0x43: ld_r_r(b, e);
    case 0x44: ld_r_r(b, h);
    case 0x45: ld_r_r(b, l);
    case 0x46: ld_r_hl(b);
    case 0x47: ld_r_r(b, a);
    case 0x48: ld_r_r(c, b);
    case 0x49: ld_r_r(c, c);
    case 0x4a: ld_r_r(c, d);
    case 0x4b: ld_r_r(c, e);
    case 0x4c: ld_r_r(c, h);
    case 0x4d: ld_r_r(c, l);
    case 0x4e: ld_r_hl(c);
    case 0x4f: ld_r_r(c, a);
    case 0x50: ld_r_r(d, b);
    case 0x51: ld_r_r(d, c);
    case 0x52: ld_r_r(d, d);
    case 0x53: ld_r_r(d, e);
    case 0x54: ld_r_r(d, h);
    case 0x55: ld_r_r(d, l);
    case 0x56: ld_r_hl(d);
    case 0x57: ld_r_r(d, a);
    case 0x58: ld_r_r(e, b);
    case 0x59: ld_r_r(e, c);
    case 0x5a: ld_r_r(e, d);
    case 0x5b: ld_r_r(e, e);
    case 0x5c: ld_r_r(e, h);
    case 0x5d: ld_r_r(e, l);
    case 0x5e: ld_r_hl(e);
    case 0x5f: ld_r_r(e, a);
    case 0x60: ld_r_r(h, b);
    case 0x61: ld_r_r(h, c);
    case 0x62: ld_r_r(h, d);
    case 0x63: ld_r_r(h, e);
    case 0x64: ld_r_r(h, h);
    case 0x65: ld_r_r(h, l);
    case 0x66: ld_r_hl(h);
    case 0x67: ld_r_r(h, a);
    case 0x68: ld_r_r(l, b);
    case 0x69: ld_r_r(l, c);
    case 0x6a: ld_r_r(l, d);
    case 0x6b: ld_r_r(l, e);
    case 0x6c: ld_r_r(l, h);
    case 0x6d: ld_r_r(l, l);
    case 0x6e: ld_r_hl(l);
    case 0x6f: ld_r_r(l, a);
    case 0x70: ld_hl_r(b);
    case 0x71: ld_hl_r(c);
    case 0x72: ld_hl_r(d);
    case 0x73: ld_hl_r(e);
    case 0x74: ld_hl_r(h);
    case 0x75: ld_hl_r(l);
    case 0x76: todo_opcode(); // halt
    case 0x77: ld_hl_r(a);
    case 0x78: ld_r_r(a, b);
    case 0x79: ld_r_r(a, c);
    case 0x7a: ld_r_r(a, d);
    case 0x7b: ld_r_r(a, e);
    case 0x7c: ld_r_r(a, h);
    case 0x7d: ld_r_r(a, l);
    case 0x7e: ld_r_hl(a);
    case 0x7f: ld_r_r(a, a);

    case 0x80: add_a_r(b);
    case 0x81: add_a_r(c);
    case 0x82: add_a_r(d);
    case 0x83: add_a_r(e);
    case 0x84: add_a_r(h);
    case 0x85: add_a_r(l);
    case 0x86: add_a_hl();
    case 0x87: add_a_r(a);

    case 0x88: adc_a_r(b);
    case 0x89: adc_a_r(c);
    case 0x8a: adc_a_r(d);
    case 0x8b: adc_a_r(e);
    case 0x8c: adc_a_r(h);
    case 0x8d: adc_a_r(l);
    case 0x8e: adc_a_hl();
    case 0x8f: adc_a_r(a);

    case 0x90: sub_a_r(b);
    case 0x91: sub_a_r(c);
    case 0x92: sub_a_r(d);
    case 0x93: sub_a_r(e);
    case 0x94: sub_a_r(h);
    case 0x95: sub_a_r(l);
    case 0x96: sub_a_hl();
    case 0x97: sub_a_r(a);

    case 0x98: todo_opcode();
    case 0x99: todo_opcode();
    case 0x9a: todo_opcode();
    case 0x9b: todo_opcode();
    case 0x9c: todo_opcode();
    case 0x9d: todo_opcode();
    case 0x9e: todo_opcode();
    case 0x9f: todo_opcode();

    case 0xa0: and_a_r(b);
    case 0xa1: and_a_r(c);
    case 0xa2: and_a_r(d);
    case 0xa3: and_a_r(e);
    case 0xa4: and_a_r(h);
    case 0xa5: and_a_r(l);
    case 0xa6: and_a_hl();
    case 0xa7: and_a_r(a);

    case 0xa8: xor_a_r(b);
    case 0xa9: xor_a_r(c);
    case 0xaa: xor_a_r(d);
    case 0xab: xor_a_r(e);
    case 0xac: xor_a_r(h);
    case 0xad: xor_a_r(l);
    case 0xae: xor_a_hl();
    case 0xaf: xor_a_r(a);

    case 0xb0: or_a_r(b);
    case 0xb1: or_a_r(c);
    case 0xb2: or_a_r(d);
    case 0xb3: or_a_r(e);
    case 0xb4: or_a_r(h);
    case 0xb5: or_a_r(l);
    case 0xb6: or_a_hl();
    case 0xb7: or_a_r(a);

    case 0xb8: cp_a_r(b);
    case 0xb9: cp_a_r(c);
    case 0xba: cp_a_r(d);
    case 0xbb: cp_a_r(e);
    case 0xbc: cp_a_r(h);
    case 0xbd: cp_a_r(l);
    case 0xbe: cp_a_hl();
    case 0xbf: cp_a_r(a);

    case 0xc0: todo_opcode();
    case 0xc1: pop_rr(bc);
    case 0xc2: todo_opcode();
    case 0xc3: todo_opcode();
    case 0xc4: todo_opcode();
    case 0xc5: push_rr(b, c);
    case 0xc6: todo_opcode();
    case 0xc7: todo_opcode();

    case 0xc8: todo_opcode();
    case 0xc9: todo_opcode();
    case 0xca: todo_opcode();
    case 0xcb: todo_opcode();
    case 0xcc: todo_opcode();
    case 0xcd: todo_opcode();
    case 0xce: todo_opcode();
    case 0xcf: todo_opcode();

    case 0xd0: todo_opcode();
    case 0xd1: pop_rr(de);
    case 0xd2: todo_opcode();
    case 0xd3: todo_opcode();
    case 0xd4: todo_opcode();
    case 0xd5: push_rr(d, e);
    case 0xd6: todo_opcode();
    case 0xd7: todo_opcode();

    case 0xd8: todo_opcode();
    case 0xd9: todo_opcode();
    case 0xda: todo_opcode();
    case 0xdb: todo_opcode();
    case 0xdc: todo_opcode();
    case 0xdd: todo_opcode();
    case 0xde: todo_opcode();
    case 0xdf: todo_opcode();

    case 0xe0: {
        u8 off = gb_mem_read(gb, gb->pc++);
        gb_mem_write(gb, 0xff00 | off, gb->a);
        return 3;
    }
    case 0xe1: pop_rr(hl);
    case 0xe2: ld_addr_a(0xff00 | gb->c);
    case 0xe3: todo_opcode();
    case 0xe4: todo_opcode();
    case 0xe5: push_rr(h, l);
    case 0xe6: todo_opcode();
    case 0xe7: todo_opcode();

    case 0xe8: todo_opcode();
    case 0xe9: todo_opcode();
    case 0xea: {
        u8 lo = gb_mem_read(gb, gb->pc++);
        u8 hi = gb_mem_read(gb, gb->pc++);
        gb_mem_write(gb, ((u16)hi << 8) | (u16)lo, gb->a);
        return 4;
    }
    case 0xeb: todo_opcode();
    case 0xec: todo_opcode();
    case 0xed: todo_opcode();
    case 0xee: todo_opcode();
    case 0xef: todo_opcode();

    case 0xf0: todo_opcode();
    case 0xf1: {
        u8 lo = gb_mem_read(gb, gb->sp++);
        u8 hi = gb_mem_read(gb, gb->sp++);
        gb->af = (((u16)hi << 8) | lo) & 0xfff0;
        return 3;
    }
    case 0xf2: ld_a_addr(0xff00 | gb->c);
    case 0xf3: todo_opcode();
    case 0xf4: todo_opcode();
    case 0xf5: push_rr(a, f);
    case 0xf6: todo_opcode();
    case 0xf7: todo_opcode();

    case 0xf8: todo_opcode();
    case 0xf9: todo_opcode();
    case 0xfa: todo_opcode();
    case 0xfb: todo_opcode();
    case 0xfc: todo_opcode();
    case 0xfd: todo_opcode();
    case 0xfe: todo_opcode();
    case 0xff: todo_opcode();
    }
}

void gb_free(GameBoy *gb) {
    if (!gb) return;
    free(gb->rom);
    gb->rom = NULL;
}

void gb_mem_write(GameBoy *gb, u16 addr, u8 val) {
    (void)gb, (void)addr, (void)val;
}

u8 gb_mem_read(GameBoy *gb, u16 addr) {
    (void)gb, (void)addr;
    return 0;
}

#define GB_IO_SB   0x01
#define GB_IO_SC   0x02

void gb_io_write(GameBoy *gb, u16 addr, u8 val) {
    u8 io_reg = addr & 0x7f;
    switch (io_reg) {
    case GB_IO_SB: gb->io_regs[io_reg] = val; break;
    case GB_IO_SC:
        val |= 2;
        gb->io_regs[io_reg] = val | (~0x83);
        if (((val & 0x80) && (val & 0x01)) && gb->serial_io) {
            gb->serial_io(gb->io_regs[GB_IO_SB]);
        }
        break;
    }
}