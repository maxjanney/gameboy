# What's left to run blargg `cpu_instrs/06-ld r,r.gb`

Reviewed against `src/gb.c` as of commit `d50a2e3` plus uncommitted work (POP/PUSH, `0x35`).

Sources: `../gb-test-roms/cpu_instrs/source/06-ld r,r.s` and `source/common/*`, cross-checked by
statically tracing every reachable instruction in the assembled ROM. The test itself only covers
`$40`–`$7F` (all done). Everything below is needed by the shared test framework (startup, CRC
tables, checksums, console, and serial output).

## 1. Bugs in existing code that will break the test

- [x] **`0xd1` doesn't compile**: `pop_rr(b, c)` should be `pop_rr(de)`.
- [x] **CP sets Z wrong** (`cp_a_r`, `cp_a_hl`): Z is set from `gb->a == 0`, but CP should set Z
      when `a - r == 0`, i.e. when `a == r`. The framework's main loop uses `CP C` (`0xb9`) to
      detect the end of the instruction list, and `CP n` (`0xfe`) is used throughout.
- [ ] **`call` pushes the wrong return address** (`call()` macro): it pushes `pc` before reading
      the 2 address bytes, so the return address points at the low address byte instead of the
      next instruction. Read `lo`/`hi` first, then push.
- [ ] **`jp_abs` / `call` don't skip the address bytes when the condition is false**: the reads
      are inside `if (cond)`, so `pc` is left on the address bytes and they get executed as
      opcodes. Read both bytes before the `if` (the first `jp_abs` in `3bf2d29` did this; it
      broke in `03c253f`).
- [x] **`0xdc` (CALL C) tests all of F**: `call(gb->f)` should be `call(gb->f & GB_FLAG_C)`.
- [x] **`0xf0` (LDH A,(a8)) never reads memory**: `a` is set to the address itself. Should be
      `gb->a = gb_mem_read(gb, 0xff00 | gb_mem_read(gb, gb->pc++));`. The framework polls LY
      (`$FF44`) with this.
- [x] **`0x1f` (RRA) doesn't rotate the old carry into bit 7**: it's a plain shift right. Should be
      `gb->a = (gb->a >> 1) | ((gb->f & GB_FLAG_C) << 3);` (flags are already correct).

## 2. Opcodes still to implement

Cycle counts are in M-cycles, the same unit as the rest of `gb_cpu_step`.

### Base opcodes

All implemented. The Nov 3 opcodes were reviewed, and the bugs found are listed in section 1.

### CB-prefixed (`CB` itself needs a second-level dispatch)

All are 2 M-cycles.

| Opcode  | Instruction | Flags                                             |
| ------- | ----------- | ------------------------------------------------- |
| `CB 19` | RR C        | Z from result, N=0, H=0, C = old bit 0            |
| `CB 1A` | RR D        | same                                              |
| `CB 1B` | RR E        | same                                              |
| `CB 37` | SWAP A      | Z from result, N=0, H=0, C=0                      |
| `CB 38` | SRL B       | Z from result, N=0, H=0, C = old bit 0, bit 7 = 0 |

## 3. Memory regions

`gb_mem_read` / `gb_mem_write` are still stubs. This is everything the test touches:

| Range       | Region | Read | Write | What the test does there                                                                                                                                                                                                  |
| ----------- | ------ | ---- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `0000–7FFF` | ROM    | ✅   | —     | Startup code at `$0100`/`$0200` in bank 0, then copies `$4000–$4FFF` to WRAM. 32 KB, no bank switching.                                                                                                                   |
| `8000–9FFF` | VRAM   | —    | ✅    | Console writes font tiles (`$8200`, `$8A00`) and the tilemap (`$9800–$9BFF`). Never read back, so writes can be dropped until there's a PPU, though `GameBoy` has no `vram` field yet.                                    |
| `C000–DFFF` | WRAM   | ✅   | ✅    | **All test code runs here.** Program at `$C000`, variables and CRC tables from `$D800`, print trampoline at `$D801`, tested instruction at `$DEF8`, stack at `$DFFF` growing down. Must support executing (`pc` in WRAM). |
| `FF80–FFFE` | HRAM   | ✅   | ✅    | CRC checksum at `$FF80–$FF83`.                                                                                                                                                                                            |
| `FFFF`      | IE     | —    | ✅    | Written as 0. Can be stored and ignored.                                                                                                                                                                                  |

Unused, can return `0xFF` and ignore writes: `A000–BFFF` (cart RAM), `E000–FDFF` (echo),
`FE00–FE9F` (OAM), `FEA0–FEFF`.

### I/O registers (`FF00–FF7F`)

| Addr                                                                                                          | Reg | R/W | Required behavior                                                                                                                         |
| ------------------------------------------------------------------------------------------------------------- | --- | --- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| `FF01`                                                                                                        | SB  | W   | Byte to send. **This is how you get output.**                                                                                             |
| `FF02`                                                                                                        | SC  | W   | `$81` starts a transfer: emit SB via `serial_io` (already in `gb_io_write`).                                                              |
| `FF44`                                                                                                        | LY  | R   | Polled for vblank (`== 144`) with a timeout, so any value works. Returning `144` skips the ~1250-iteration timeout on every console call. |
| `FF07` TAC, `FF0F` IF, `FF24` NR50, `FF25` NR51, `FF26` NR52, `FF40` LCDC, `FF42` SCY, `FF43` SCX, `FF47` BGP |     | W   | Write-only setup. Storing them in `io_regs` (or ignoring them) is fine.                                                                   |

`gb_io_write` exists but isn't called from `gb_mem_write` yet, and there's no `gb_io_read` for LY.
