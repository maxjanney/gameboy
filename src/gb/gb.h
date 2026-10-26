#define GB_IO_SB   0x01
#define GB_IO_SC   0x02
#define GB_IO_REGS 0x80

typedef struct GameBoy GameBoy;
typedef void(*SerialCallback)(u8);

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
    u8 *rom;
    u8 io_regs[GB_IO_REGS];

    SerialCallback serial_io;
};

void gb_init(GameBoy*);
int gb_load_rom(GameBoy*, const char*);
void gb_free(GameBoy*);

int gb_cpu_step(GameBoy*);

void gb_mem_write(GameBoy*, u16, u8);
u8 gb_mem_read(GameBoy*, u16);
void gb_io_write(GameBoy*, u16, u8);