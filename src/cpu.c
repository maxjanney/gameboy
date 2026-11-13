#define GB_FLAG_Z (1 << 7)
#define GB_FLAG_N (1 << 6)
#define GB_FLAG_H (1 << 5)
#define GB_FLAG_C (1 << 4)

#define illegal_opcode() {                                                              \
    fprintf(stderr, "Illegal opcode encountered: 0x%02X on line %d\n", op, __LINE__);   \
    abort();                                                                            \
}

#define todo_opcode() {                                                                     \
    fprintf(stderr, "Unimplemented opcode encountered: 0x%02X on line %d\n", op, __LINE__); \
    abort();                                                                                \
}

#define rlc_r(r) {                          \
    u8 c = gb->r >> 7;                      \
    gb->r = (gb->r << 1) | c;               \
    gb->f = ((gb->r == 0) << 7) | (c << 4); \
    return 2;                               \
}

#define rr_r(r) {                                       \
    u8 b = gb->r & 0x01;                                \
    gb->r = ((gb->f & GB_FLAG_C) << 3) | (gb->r >> 1);  \
    gb->f = ((gb->r == 0) << 7) | (b << 4);             \
    return 2;                                           \
}

#define swap_r(r) {                     \
    u8 lo = gb->r & 0xf;                \
    gb->r = (lo << 4) | (gb->r >> 4);   \
    gb->f = (gb->r == 0) << 7;          \
    return 2;                           \
}

#define srl_r(r) {                          \
    u8 c = gb->r & 0x01;                    \
    gb->r = gb->r >> 1;                     \
    gb->f = ((gb->r == 0) << 7) | (c << 4); \
    return 2;                               \
}

#define rrc_r(r) {                          \
    u8 c = (gb->r & 0x01);                  \
    gb->r = (c << 7) | (gb->r >> 1);        \
    gb->f = ((gb->r == 0) << 7) | (c << 4); \
    return 2;                               \
}

#define rl_r(r) {                                       \
    u8 c = gb->r >> 7;                                  \
    gb->r = (gb->r << 1) | ((gb->f & GB_FLAG_C) >> 4);  \
    gb->f = ((gb->r == 0) << 7) | (c << 4);             \
    return 2;                                           \
}

#define sla_r(r) {                          \
    u8 c = gb->r >> 7;                      \
    gb->r <<= 1;                            \
    gb->f = ((gb->r == 0) << 7) | (c << 4); \
    return 2;                               \
}

#define sra_r(r) {\
    u8 c = gb->r & 0x01;\
    gb->r = (gb->r & 0x80) | (gb->r >> 1);\
    gb->f = ((gb->r == 0) << 7) | (c << 4);\
    return 2;\
}

#define bit_n(r, n) {                                                       \
    gb->f = (!(gb->r & (1 << (n))) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C); \
    return 2;                                                               \
}

#define res_n(r, n) {   \
    gb->r &= ~(1 << n); \
    return 2;           \
}

#define set_n(r, n) {   \
    gb->r |= 1 << n;    \
    return 2;           \
}

static int gb_cb_prefix(GameBoy *gb) {
    u8 op = gb_mem_read(gb, gb->pc++);
    switch (op) {
    case 0x00: rlc_r(b);
    case 0x01: rlc_r(c);
    case 0x02: rlc_r(d);
    case 0x03: rlc_r(e);
    case 0x04: rlc_r(h);
    case 0x05: rlc_r(l);
    case 0x06: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v >> 7;
        v = (v << 1) | c;
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x07: rlc_r(a);

    case 0x08: rrc_r(b);
    case 0x09: rrc_r(c);
    case 0x0a: rrc_r(d);
    case 0x0b: rrc_r(e);
    case 0x0c: rrc_r(h);
    case 0x0d: rrc_r(l);
    case 0x0e: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v & 0x01;
        v = (c << 7) | (v >> 1);
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x0f: rrc_r(a);

    case 0x10: rl_r(b);
    case 0x11: rl_r(c);
    case 0x12: rl_r(d);
    case 0x13: rl_r(e);
    case 0x14: rl_r(h);
    case 0x15: rl_r(l);
    case 0x16: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v >> 7;
        v = (v << 1) | ((gb->f & GB_FLAG_C) >> 4);
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x17: rl_r(a);

    case 0x18: rr_r(b);
    case 0x19: rr_r(c);
    case 0x1a: rr_r(d);
    case 0x1b: rr_r(e);
    case 0x1c: rr_r(h);
    case 0x1d: rr_r(l);
    case 0x1e: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v & 0x01;
        v = ((gb->f & GB_FLAG_C) << 3) | (v >> 1);
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x1f: rr_r(a);

    case 0x20: sla_r(b);
    case 0x21: sla_r(c);
    case 0x22: sla_r(d);
    case 0x23: sla_r(e);
    case 0x24: sla_r(h);
    case 0x25: sla_r(l);
    case 0x26: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v >> 7;
        v <<= 1;
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x27: sla_r(a);

    case 0x28: sra_r(b);
    case 0x29: sra_r(c);
    case 0x2a: sra_r(d);
    case 0x2b: sra_r(e);
    case 0x2c: sra_r(h);
    case 0x2d: sra_r(l);
    case 0x2e: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v & 0x01;
        v = (v & 0x80) | (v >> 1);
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x2f: sra_r(a);

    case 0x30: swap_r(b);
    case 0x31: swap_r(c);
    case 0x32: swap_r(d);
    case 0x33: swap_r(e);
    case 0x34: swap_r(h);
    case 0x35: swap_r(l);
    case 0x36: {
        u8 v = gb_mem_read(gb, gb->hl);
        v = (v << 4) | (v >> 4);
        gb->f = (v == 0) << 7;
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x37: swap_r(a);

    case 0x38: srl_r(b);
    case 0x39: srl_r(c);
    case 0x3a: srl_r(d);
    case 0x3b: srl_r(e);
    case 0x3c: srl_r(h);
    case 0x3d: srl_r(l);
    case 0x3e: {
        u8 v = gb_mem_read(gb, gb->hl);
        u8 c = v & 0x01;
        v >>= 1;
        gb->f = ((v == 0) << 7) | (c << 4);
        gb_mem_write(gb, gb->hl, v);
        return 4;
    }
    case 0x3f: srl_r(a);

    case 0x40: bit_n(b, 0);
    case 0x41: bit_n(c, 0);
    case 0x42: bit_n(d, 0);
    case 0x43: bit_n(e, 0);
    case 0x44: bit_n(h, 0);
    case 0x45: bit_n(l, 0);
    case 0x46: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x01) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x47: bit_n(a, 0);

    case 0x48: bit_n(b, 1);
    case 0x49: bit_n(c, 1);
    case 0x4a: bit_n(d, 1);
    case 0x4b: bit_n(e, 1);
    case 0x4c: bit_n(h, 1);
    case 0x4d: bit_n(l, 1);
    case 0x4e: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x02) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x4f: bit_n(a, 1);

    case 0x50: bit_n(b, 2);
    case 0x51: bit_n(c, 2);
    case 0x52: bit_n(d, 2);
    case 0x53: bit_n(e, 2);
    case 0x54: bit_n(h, 2);
    case 0x55: bit_n(l, 2);
    case 0x56: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x04) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x57: bit_n(a, 2);

    case 0x58: bit_n(b, 3);
    case 0x59: bit_n(c, 3);
    case 0x5a: bit_n(d, 3);
    case 0x5b: bit_n(e, 3);
    case 0x5c: bit_n(h, 3);
    case 0x5d: bit_n(l, 3);
    case 0x5e: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x08) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x5f: bit_n(a, 3);

    case 0x60: bit_n(b, 4);
    case 0x61: bit_n(c, 4);
    case 0x62: bit_n(d, 4);
    case 0x63: bit_n(e, 4);
    case 0x64: bit_n(h, 4);
    case 0x65: bit_n(l, 4);
    case 0x66: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x10) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x67: bit_n(a, 4);

    case 0x68: bit_n(b, 5);
    case 0x69: bit_n(c, 5);
    case 0x6a: bit_n(d, 5);
    case 0x6b: bit_n(e, 5);
    case 0x6c: bit_n(h, 5);
    case 0x6d: bit_n(l, 5);
    case 0x6e: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x20) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x6f: bit_n(a, 5);

    case 0x70: bit_n(b, 6);
    case 0x71: bit_n(c, 6);
    case 0x72: bit_n(d, 6);
    case 0x73: bit_n(e, 6);
    case 0x74: bit_n(h, 6);
    case 0x75: bit_n(l, 6);
    case 0x76: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x40) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x77: bit_n(a, 6);

    case 0x78: bit_n(b, 7);
    case 0x79: bit_n(c, 7);
    case 0x7a: bit_n(d, 7);
    case 0x7b: bit_n(e, 7);
    case 0x7c: bit_n(h, 7);
    case 0x7d: bit_n(l, 7);
    case 0x7e: {
        gb->f = (!(gb_mem_read(gb, gb->hl) & 0x80) << 7) | GB_FLAG_H | (gb->f & GB_FLAG_C);
        return 3;
    }
    case 0x7f: bit_n(a, 7);

    case 0x80: res_n(b, 0);
    case 0x81: res_n(c, 0);
    case 0x82: res_n(d, 0);
    case 0x83: res_n(e, 0);
    case 0x84: res_n(h, 0);
    case 0x85: res_n(l, 0);
    case 0x86: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x01);
        return 4;
    }
    case 0x87: res_n(a, 0);

    case 0x88: res_n(b, 1);
    case 0x89: res_n(c, 1);
    case 0x8a: res_n(d, 1);
    case 0x8b: res_n(e, 1);
    case 0x8c: res_n(h, 1);
    case 0x8d: res_n(l, 1);
    case 0x8e: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x02);
        return 4;
    }
    case 0x8f: res_n(a, 1);

    case 0x90: res_n(b, 2);
    case 0x91: res_n(c, 2);
    case 0x92: res_n(d, 2);
    case 0x93: res_n(e, 2);
    case 0x94: res_n(h, 2);
    case 0x95: res_n(l, 2);
    case 0x96: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x04);
        return 4;
    }
    case 0x97: res_n(a, 2);

    case 0x98: res_n(b, 3);
    case 0x99: res_n(c, 3);
    case 0x9a: res_n(d, 3);
    case 0x9b: res_n(e, 3);
    case 0x9c: res_n(h, 3);
    case 0x9d: res_n(l, 3);
    case 0x9e: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x08);
        return 4;
    }
    case 0x9f: res_n(a, 3);

    case 0xa0: res_n(b, 4);
    case 0xa1: res_n(c, 4);
    case 0xa2: res_n(d, 4);
    case 0xa3: res_n(e, 4);
    case 0xa4: res_n(h, 4);
    case 0xa5: res_n(l, 4);
    case 0xa6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x10);
        return 4;
    }
    case 0xa7: res_n(a, 4);

    case 0xa8: res_n(b, 5);
    case 0xa9: res_n(c, 5);
    case 0xaa: res_n(d, 5);
    case 0xab: res_n(e, 5);
    case 0xac: res_n(h, 5);
    case 0xad: res_n(l, 5);
    case 0xae: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x20);
        return 4;
    }
    case 0xaf: res_n(a, 5);

    case 0xb0: res_n(b, 6);
    case 0xb1: res_n(c, 6);
    case 0xb2: res_n(d, 6);
    case 0xb3: res_n(e, 6);
    case 0xb4: res_n(h, 6);
    case 0xb5: res_n(l, 6);
    case 0xb6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x40);
        return 4;
    }

    case 0xb7: res_n(a, 6);

    case 0xb8: res_n(b, 7);
    case 0xb9: res_n(c, 7);
    case 0xba: res_n(d, 7);
    case 0xbb: res_n(e, 7);
    case 0xbc: res_n(h, 7);
    case 0xbd: res_n(l, 7);
    case 0xbe: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) & ~0x80);
        return 4;
    }
    case 0xbf: res_n(a, 7);

    case 0xc0: set_n(b, 0);
    case 0xc1: set_n(c, 0);
    case 0xc2: set_n(d, 0);
    case 0xc3: set_n(e, 0);
    case 0xc4: set_n(h, 0);
    case 0xc5: set_n(l, 0);
    case 0xc6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x01);
        return 4;
    }
    case 0xc7: set_n(a, 0);

    case 0xc8: set_n(b, 1);
    case 0xc9: set_n(c, 1);
    case 0xca: set_n(d, 1);
    case 0xcb: set_n(e, 1);
    case 0xcc: set_n(h, 1);
    case 0xcd: set_n(l, 1);
    case 0xce: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x02);
        return 4;
    }
    case 0xcf: set_n(a, 1);

    case 0xd0: set_n(b, 2);
    case 0xd1: set_n(c, 2);
    case 0xd2: set_n(d, 2);
    case 0xd3: set_n(e, 2);
    case 0xd4: set_n(h, 2);
    case 0xd5: set_n(l, 2);
    case 0xd6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x04);
        return 4;
    }
    case 0xd7: set_n(a, 2);

    case 0xd8: set_n(b, 3);
    case 0xd9: set_n(c, 3);
    case 0xda: set_n(d, 3);
    case 0xdb: set_n(e, 3);
    case 0xdc: set_n(h, 3);
    case 0xdd: set_n(l, 3);
    case 0xde: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x08);
        return 4;
    }

    case 0xdf: set_n(a, 3);

    case 0xe0: set_n(b, 4);
    case 0xe1: set_n(c, 4);
    case 0xe2: set_n(d, 4);
    case 0xe3: set_n(e, 4);
    case 0xe4: set_n(h, 4);
    case 0xe5: set_n(l, 4);
    case 0xe6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x10);
        return 4;
    }

    case 0xe7: set_n(a, 4);

    case 0xe8: set_n(b, 5);
    case 0xe9: set_n(c, 5);
    case 0xea: set_n(d, 5);
    case 0xeb: set_n(e, 5);
    case 0xec: set_n(h, 5);
    case 0xed: set_n(l, 5);
    case 0xee: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x20);
        return 4;
    }

    case 0xef: set_n(a, 5);

    case 0xf0: set_n(b, 6);
    case 0xf1: set_n(c, 6);
    case 0xf2: set_n(d, 6);
    case 0xf3: set_n(e, 6);
    case 0xf4: set_n(h, 6);
    case 0xf5: set_n(l, 6);
    case 0xf6: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x40);
        return 4;
    }

    case 0xf7: set_n(a, 6);

    case 0xf8: set_n(b, 7);
    case 0xf9: set_n(c, 7);
    case 0xfa: set_n(d, 7);
    case 0xfb: set_n(e, 7);
    case 0xfc: set_n(h, 7);
    case 0xfd: set_n(l, 7);
    case 0xfe: {
        gb_mem_write(gb, gb->hl, gb_mem_read(gb, gb->hl) | 0x80);
        return 4;
    }
    case 0xff: set_n(a, 7);
    }
    return 0;
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

#define add_a(val_expr, cycles) {               \
    u8 val = (val_expr);                        \
    u16 res = gb->a + val;                      \
    u8 h = (gb->a & 0xf) + (val & 0xf) > 0xf;   \
    gb->a = (u8)res;                            \
    gb->f = ((gb->a == 0) << 7) | /* Z */       \
            /* N = 0 */                         \
            (h << 5) | /* H */                  \
            ((res > 0xff) << 4); /* C */        \
    return (cycles);                            \
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

#define adc_a(val_expr, cycles) {                       \
    u8 val = (val_expr);                                \
    u8 c = (gb->f & GB_FLAG_C) >> 4;                    \
    u16 res = gb->a + val + c;                          \
    u8 h = (gb->a & 0xf) + (val & 0xf) + c > 0xf;       \
    gb->a = (u8)res;                                    \
    gb->f = ((gb->a == 0) << 7) | /* Z */               \
            /* N = 0 */                                 \
            (h << 5) | /* H */                          \
            ((res > 0xff) << 4); /* C */                \
    return (cycles);                                    \
}

#define sub_a(val_expr, cycles) {                       \
    u8 val = (val_expr);                                \
    u8 c = val > gb->a;                                 \
    u8 h = (((gb->a & 0xf) - (val & 0xf)) & 0x10) != 0; \
    gb->a -= val;                                       \
    gb->f = ((gb->a == 0) << 7) | /* Z */               \
            GB_FLAG_N | /* N = 1 */                     \
            (h << 5) | /* H */                          \
            (c << 4); /* C */                           \
    return (cycles);                                    \
}

#define sbc_a(val_expr, cycles) {                                       \
    u8 val = (val_expr);                                                \
    u8 c = (gb->f & GB_FLAG_C) >> 4;                                    \
    int cf = gb->a < val + c;                                           \
    int hf = (gb->a & 0xf) < (val & 0xf) + c;                           \
    gb->a = gb->a - val - c;                                            \
    gb->f = ((gb->a == 0) << 7) | GB_FLAG_N | (hf << 5) | (cf << 4);    \
    return (cycles);                                                    \
}

#define and_a(val_expr, cycles) {               \
    gb->a &= (val_expr);                        \
    gb->f = ((gb->a == 0) << 7) | GB_FLAG_H;    \
    return (cycles);                            \
}

#define xor_a(val_expr, cycles) {   \
    gb->a ^= (val_expr);            \
    gb->f = (gb->a == 0) << 7;      \
    return (cycles);                \
}

#define or_a(val_expr, cycles) {    \
    gb->a |= (val_expr);            \
    gb->f = (gb->a == 0) << 7;      \
    return (cycles);                \
}

#define cp_a(val_expr, cycles) {                        \
    u8 val = (val_expr);                                \
    u8 c = val > gb->a;                                 \
    u8 h = (((gb->a & 0xf) - (val & 0xf)) & 0x10) != 0; \
    gb->f = ((gb->a == val) << 7) | /* Z */             \
            GB_FLAG_N | /* N = 1 */                     \
            (h << 5) | /* H */                          \
            (c << 4); /* C */                           \
    return (cycles);                                    \
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
    gb->rr = ((u16)hi << 8) | lo;       \
    return 3;                           \
}

#define push_rr(hi, lo) {               \
    gb_mem_write(gb, --gb->sp, gb->hi); \
    gb_mem_write(gb, --gb->sp, gb->lo); \
    return 4;                           \
}

#define jp_abs(cond) {                      \
    if (cond) {                             \
        u8 lo = gb_mem_read(gb, gb->pc++);  \
        u8 hi = gb_mem_read(gb, gb->pc++);  \
        gb->pc = ((u16)hi << 8) | lo;       \
        return 4;                           \
    }                                       \
    gb->pc += 2;                            \
    return 3;                               \
}

#define call(cond) {                                \
    if (cond) {                                     \
        u8 lo = gb_mem_read(gb, gb->pc++);          \
        u8 hi = gb_mem_read(gb, gb->pc++);          \
        gb_mem_write(gb, --gb->sp, gb->pc >> 8);    \
        gb_mem_write(gb, --gb->sp, gb->pc);         \
        gb->pc = ((u16)hi << 8) | lo;               \
        return 6;                                   \
    }                                               \
    gb->pc += 2;                                    \
    return 3;                                       \
}

#define rst(vec) {                                  \
    gb_mem_write(gb, --gb->sp, gb->pc >> 8);        \
    gb_mem_write(gb, --gb->sp, gb->pc);             \
    gb->pc = (vec);                                 \
    return 4;                                       \
}

#define ret(cond) {                         \
    if (cond) {                             \
        u8 lo = gb_mem_read(gb, gb->sp++);  \
        u8 hi = gb_mem_read(gb, gb->sp++);  \
        gb->pc = ((u16)hi << 8) | lo;       \
        return 5;                           \
    }                                       \
    return 2;                               \
}

static u16 interrupt_addr_map[] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};

static int gb_cpu_interrupt(GameBoy *gb) {
    gb->ime = 0;
    int interrupt = __builtin_ctz(gb->interrupt_enable & gb->io_regs[GB_IO_IF] & 0x1f);
    u16 interrupt_addr = interrupt_addr_map[interrupt];
    gb->io_regs[GB_IO_IF] &= ~(1 << interrupt);

    gb_mem_write(gb, --gb->sp, gb->pc >> 8);
    gb_mem_write(gb, --gb->sp, gb->pc);

    gb->pc = interrupt_addr;
    return 5;
}

static int gb_cpu_step_mcycles(GameBoy *gb) {
    u8 pending_interrupts = (gb->interrupt_enable & gb->io_regs[GB_IO_IF] & 0x1f) != 0;
    if (gb->halted && pending_interrupts) {
        gb->halted = 0;
    } else if (gb->halted) {
        return 1;
    }

    u8 ime_delay = gb->ime_delay;
    gb->ime_delay = 0;

    if (gb->ime & pending_interrupts) {
        return gb_cpu_interrupt(gb);
    }

    if (ime_delay) gb->ime = 1;

    u8 op = gb_mem_read(gb, gb->pc++);
    switch (op) {
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

    case 0x10: {
        gb->io_regs[GB_IO_DIV] = 0;
        gb->stopped = 1;
        gb->pc++;
        return 1;
    }
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
        gb->a = (gb->a >> 1) | ((gb->f & GB_FLAG_C) << 3);
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
    case 0x27: {
        u8 offset = 0;
        u8 carry = 0;
        u8 add = !(gb->f & GB_FLAG_N);
        if ((add && ((gb->a & 0xf) > 0x9)) || (gb->f & GB_FLAG_H)) {
            offset |= 0x06;
        }

        if ((add && (gb->a > 0x99)) || (gb->f & GB_FLAG_C)) {
            offset |= 0x60;
            carry = 1;
        }

        if (add) {
            gb->a += offset;
        } else {
            gb->a -= offset;
        }

        gb->f = ((gb->a == 0) << 7) | (gb->f & GB_FLAG_N) | (carry << 4);
        return 1;
    }
    case 0x28: jr_rel(gb->f & GB_FLAG_Z);
    case 0x29: add_hl_rr(hl);
    case 0x2a: ld_a_addr(gb->hl++);
    case 0x2b: dec_rr(hl);
    case 0x2c: inc_r(l);
    case 0x2d: dec_r(l);
    case 0x2e: ld_r_u8(l);
    case 0x2f: {
        gb->a = ~gb->a;
        gb->f |= GB_FLAG_N | GB_FLAG_H;
        return 1;
    }

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
    case 0x37: {
        gb->f = (gb->f & GB_FLAG_Z) | GB_FLAG_C;
        return 1;
    }
    case 0x38: jr_rel(gb->f & GB_FLAG_C);
    case 0x39: add_hl_rr(sp);
    case 0x3a: ld_a_addr(gb->hl--);
    case 0x3b: dec_rr(sp);
    case 0x3c: inc_r(a);
    case 0x3d: dec_r(a);
    case 0x3e: ld_r_u8(a);
    case 0x3f: {
        gb->f = (gb->f & GB_FLAG_Z) | (~gb->f & GB_FLAG_C);
        return 1;
    }

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
    case 0x76: {
        // before executing a halt, we should check for serviciable interrupts
        // if there are any and IME=0 (halt bug) and if IME=1, exit and the next loop iter handles
        // the interrupt if there are no servicable interrupts, then we enter a halted state
        if ((gb->interrupt_enable & gb->io_regs[GB_IO_IF] & 0x1f) != 0) {
            gb->halted = 0;
        } else {
            gb->halted = 1;
        }
        return 1;
    }
    case 0x77: ld_hl_r(a);
    case 0x78: ld_r_r(a, b);
    case 0x79: ld_r_r(a, c);
    case 0x7a: ld_r_r(a, d);
    case 0x7b: ld_r_r(a, e);
    case 0x7c: ld_r_r(a, h);
    case 0x7d: ld_r_r(a, l);
    case 0x7e: ld_r_hl(a);
    case 0x7f: ld_r_r(a, a);

    case 0x80: add_a(gb->b, 1);
    case 0x81: add_a(gb->c, 1);
    case 0x82: add_a(gb->d, 1);
    case 0x83: add_a(gb->e, 1);
    case 0x84: add_a(gb->h, 1);
    case 0x85: add_a(gb->l, 1);
    case 0x86: add_a(gb_mem_read(gb, gb->hl), 2);
    case 0x87: add_a(gb->a, 1);

    case 0x88: adc_a(gb->b, 1);
    case 0x89: adc_a(gb->c, 1);
    case 0x8a: adc_a(gb->d, 1);
    case 0x8b: adc_a(gb->e, 1);
    case 0x8c: adc_a(gb->h, 1);
    case 0x8d: adc_a(gb->l, 1);
    case 0x8e: adc_a(gb_mem_read(gb, gb->hl), 2);
    case 0x8f: adc_a(gb->a, 1);

    case 0x90: sub_a(gb->b, 1);
    case 0x91: sub_a(gb->c, 1);
    case 0x92: sub_a(gb->d, 1);
    case 0x93: sub_a(gb->e, 1);
    case 0x94: sub_a(gb->h, 1);
    case 0x95: sub_a(gb->l, 1);
    case 0x96: sub_a(gb_mem_read(gb, gb->hl), 2);
    case 0x97: sub_a(gb->a, 1);

    case 0x98: sbc_a(gb->b, 1);
    case 0x99: sbc_a(gb->c, 1);
    case 0x9a: sbc_a(gb->d, 1);
    case 0x9b: sbc_a(gb->e, 1);
    case 0x9c: sbc_a(gb->h, 1);
    case 0x9d: sbc_a(gb->l, 1);
    case 0x9e: sbc_a(gb_mem_read(gb, gb->hl), 2);
    case 0x9f: sbc_a(gb->a, 1);

    case 0xa0: and_a(gb->b, 1);
    case 0xa1: and_a(gb->c, 1);
    case 0xa2: and_a(gb->d, 1);
    case 0xa3: and_a(gb->e, 1);
    case 0xa4: and_a(gb->h, 1);
    case 0xa5: and_a(gb->l, 1);
    case 0xa6: and_a(gb_mem_read(gb, gb->hl), 2);
    case 0xa7: and_a(gb->a, 1);

    case 0xa8: xor_a(gb->b, 1);
    case 0xa9: xor_a(gb->c, 1);
    case 0xaa: xor_a(gb->d, 1);
    case 0xab: xor_a(gb->e, 1);
    case 0xac: xor_a(gb->h, 1);
    case 0xad: xor_a(gb->l, 1);
    case 0xae: xor_a(gb_mem_read(gb, gb->hl), 2);
    case 0xaf: xor_a(gb->a, 1);

    case 0xb0: or_a(gb->b, 1);
    case 0xb1: or_a(gb->c, 1);
    case 0xb2: or_a(gb->d, 1);
    case 0xb3: or_a(gb->e, 1);
    case 0xb4: or_a(gb->h, 1);
    case 0xb5: or_a(gb->l, 1);
    case 0xb6: or_a(gb_mem_read(gb, gb->hl), 2);
    case 0xb7: or_a(gb->a, 1);

    case 0xb8: cp_a(gb->b, 1);
    case 0xb9: cp_a(gb->c, 1);
    case 0xba: cp_a(gb->d, 1);
    case 0xbb: cp_a(gb->e, 1);
    case 0xbc: cp_a(gb->h, 1);
    case 0xbd: cp_a(gb->l, 1);
    case 0xbe: cp_a(gb_mem_read(gb, gb->hl), 2);
    case 0xbf: cp_a(gb->a, 1);

    case 0xc0: ret(!(gb->f & GB_FLAG_Z));
    case 0xc1: pop_rr(bc);
    case 0xc2: jp_abs(!(gb->f & GB_FLAG_Z));
    case 0xc3: jp_abs(1);
    case 0xc4: call(!(gb->f & GB_FLAG_Z));
    case 0xc5: push_rr(b, c);
    case 0xc6: add_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xc7: rst(0x00);

    case 0xc8: ret(gb->f & GB_FLAG_Z);
    case 0xc9: {
        u8 lo = gb_mem_read(gb, gb->sp++);
        u8 hi = gb_mem_read(gb, gb->sp++);
        gb->pc = ((u16)hi << 8) | lo;
        return 4;
    }
    case 0xca: jp_abs(gb->f & GB_FLAG_Z);
    case 0xcb: return gb_cb_prefix(gb);
    case 0xcc: call(gb->f & GB_FLAG_Z);
    case 0xcd: call(1);
    case 0xce: adc_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xcf: rst(0x08);

    case 0xd0: ret(!(gb->f & GB_FLAG_C));
    case 0xd1: pop_rr(de);
    case 0xd2: jp_abs(!(gb->f & GB_FLAG_C));
    case 0xd3: illegal_opcode();
    case 0xd4: call(!(gb->f & GB_FLAG_C));
    case 0xd5: push_rr(d, e);
    case 0xd6: sub_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xd7: rst(0x10);

    case 0xd8: ret(gb->f & GB_FLAG_C);
    case 0xd9: {
        u16 lo = (u16)gb_mem_read(gb, gb->sp++);
        u16 hi = (u16)gb_mem_read(gb, gb->sp++);
        gb->pc = (hi << 8) | lo;
        gb->ime = 1;
        return 4;
    }
    case 0xda: jp_abs(gb->f & GB_FLAG_C);
    case 0xdb: illegal_opcode();
    case 0xdc: call(gb->f & GB_FLAG_C);
    case 0xdd: illegal_opcode();
    case 0xde: {
        u8 n = gb_mem_read(gb, gb->pc++);
        u8 c = (gb->f & GB_FLAG_C) >> 4;
        int cf = gb->a < (n + c);
        int hf = (gb->a & 0xf) < (n & 0xf) + c;
        gb->a = gb->a - n - c;
        gb->f = ((gb->a == 0) << 7) | GB_FLAG_N | (hf << 5) | (cf << 4);
        return 2;
    }
    case 0xdf: rst(0x18);

    case 0xe0: {
        u8 off = gb_mem_read(gb, gb->pc++);
        gb_mem_write(gb, 0xff00 | off, gb->a);
        return 3;
    }
    case 0xe1: pop_rr(hl);
    case 0xe2: ld_addr_a(0xff00 | gb->c);
    case 0xe3: illegal_opcode();
    case 0xe4: illegal_opcode();
    case 0xe5: push_rr(h, l);
    case 0xe6: and_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xe7: rst(0x20);

    case 0xe8: {
        u8 u = gb_mem_read(gb, gb->pc++);
        u8 h = (gb->sp & 0xf) + (u & 0xf) > 0xf;
        u8 c = (gb->sp & 0xff) + u > 0xff;
        gb->sp = gb->sp + (i8)u;
        gb->f = (h << 5) | (c << 4);
        return 4;
    }
    case 0xe9: gb->pc = gb->hl; return 1;
    case 0xea: {
        u8 lo = gb_mem_read(gb, gb->pc++);
        u8 hi = gb_mem_read(gb, gb->pc++);
        gb_mem_write(gb, ((u16)hi << 8) | (u16)lo, gb->a);
        return 4;
    }
    case 0xeb: illegal_opcode();
    case 0xec: illegal_opcode();
    case 0xed: illegal_opcode();
    case 0xee: xor_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xef: rst(0x28);

    case 0xf0: {
        gb->a = gb_mem_read(gb, 0xff00 | (u16)gb_mem_read(gb, gb->pc++));
        return 3;
    }
    case 0xf1: {
        u8 lo = gb_mem_read(gb, gb->sp++);
        u8 hi = gb_mem_read(gb, gb->sp++);
        gb->af = (((u16)hi << 8) | lo) & 0xfff0;
        return 3;
    }
    case 0xf2: ld_a_addr(0xff00 | gb->c);
    case 0xf3: gb->ime = gb->ime_delay = 0; return 1;
    case 0xf4: illegal_opcode();
    case 0xf5: push_rr(a, f);
    case 0xf6: or_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xf7: rst(0x30);

    case 0xf8: {
        u8 u = gb_mem_read(gb, gb->pc++);
        u8 h = (gb->sp & 0xf) + (u & 0xf) > 0xf;
        u8 c = (gb->sp & 0xff) + u > 0xff;
        gb->hl = gb->sp + (i8)u;
        gb->f = (h << 5) | (c << 4);
        return 3;
    }
    case 0xf9: {
        gb->sp = gb->hl;
        return 2;
    }
    case 0xfa: {
        u8 lo = gb_mem_read(gb, gb->pc++);
        u8 hi = gb_mem_read(gb, gb->pc++);
        gb->a = gb_mem_read(gb, ((u16)hi << 8) | lo);
        return 4;
    }
    case 0xfb: gb->ime_delay = 1; return 1;
    case 0xfc: illegal_opcode();
    case 0xfd: illegal_opcode();
    case 0xfe: cp_a(gb_mem_read(gb, gb->pc++), 2);
    case 0xff: rst(0x38);
    default: todo_opcode();
    }
}

int gb_cpu_step(GameBoy *gb) {
    return gb_cpu_step_mcycles(gb) * 4;
}
