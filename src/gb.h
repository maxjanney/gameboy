#define TCYCLES_PER_FRAME 70224

typedef void (*SerialCallback)(u8);

typedef enum MBCType MBCType;
enum MBCType {
    MBC_NONE,
    MBC_1,
    MBC_3,
    MBC_5,
};

typedef struct GameBoy GameBoy;
struct GameBoy {
    union {
        struct {
            u8 f, a;
            u8 c, b;
            u8 e, d;
            u8 l, h;
        };
        struct {
            u16 af;
            u16 bc;
            u16 de;
            u16 hl;
            u16 pc;
            u16 sp;
        };
    };

    u8 ime;
    u8 ime_delay;
    u8 interrupt_enable;

    u8 halted;
    u8 stopped;

    u16 div_counter;

    u8 halt_bug;

    // todo: parse from header
    MBCType mbc_ty;
    union {
        struct {
            unsigned int rom_bank : 5;
            unsigned int ram_bank : 2;
            unsigned int mode : 1;
        } mbc1;
    };
    bool32 ram_enable;

    u8 vram[0x2000];
    u8 eram[0x2000];
    u8 oam[0xa0];
    u8 wram[0x2000];
    u8 hram[0x7f];

    u32 ppu_ticks;

    u8 *rom;
    size_t rom_len;

    u8 io_regs[0x80];
    SerialCallback serial_io;
};

void gb_init(GameBoy *, SerialCallback);
int gb_load_rom(GameBoy *, const char *);
void gb_free(GameBoy *);

int gb_cpu_step(GameBoy *);

void gb_mem_write(GameBoy *, u16, u8);
u8 gb_mem_read(GameBoy *, u16);

void gb_timer_tick(GameBoy *, int);
void gb_ppu_tick(GameBoy *, int);
