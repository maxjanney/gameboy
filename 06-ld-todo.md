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

## 2. Opcodes still to implement

Cycle counts are in M-cycles, the same unit as the rest of `gb_cpu_step`.

### Base opcodes

Check with cc to review the implemented instructions
| Opcode | Instruction | Cycles | Notes |
| ------ | ----------- | ------ | --------------------------------------------------------------------------- |
| `CD` | CALL nn | 6 | Push `pc` (after the operand), then jump. |
| `C4` | CALL NZ,nn | 6 / 3 | taken / not taken |
| `D4` | CALL NC,nn | 6 / 3 | taken / not taken |
| `C9` | RET | 4 | |
| `C8` | RET Z | 5 / 2 | taken / not taken |
| `D0` | RET NC | 5 / 2 | |
| `D8` | RET C | 5 / 2 | |
| `C6` | ADD A,n | 2 | same flags as `add_a_r` |
| `CE` | ADC A,n | 2 | same flags as `adc_a_r` |
| `D6` | SUB n | 2 | same flags as `sub_a_r` |
| `E6` | AND n | 2 | Z, N=0, H=1, C=0 |
| `EE` | XOR n | 2 | Z, others 0 |
| `FE` | CP n | 2 | like SUB but `a` unchanged (Z = `a == n`) |
| `F0` | LDH A,(n) | 3 | `a = read(0xff00 \| n)` |
| `FA` | LD A,(nn) | 4 | |
| `F3` | DI | 1 | Can be a no-op until interrupts exist. |

### CB-prefixed (`CB` itself needs a second-level dispatch)

All are 2 M-cycles.

| Opcode  | Instruction | Flags                                             |
| ------- | ----------- | ------------------------------------------------- |
| `CB 19` | RR C        | Z from result, N=0, H=0, C = old bit 0            |
| `CB 1A` | RR D        | same                                              |
| `CB 1B` | RR E        | same                                              |
| `CB 37` | SWAP A      | Z from result, N=0, H=0, C=0                      |
| `CB 38` | SRL B       | Z from result, N=0, H=0, C = old bit 0, bit 7 = 0 |

### Already done (needed by the test)

`00-0F`, `11-1E`, `20-3F`, `40-7F` (except `76`), `81`, `91`, `A9`, `AE`, `AF`, `B0`, `B1`, `B6`,
`B7`, `B9`, `C1`, `C5`, `D5`, `E0`, `E1`, `E5`, `EA`, `F1`, `F5`

### Not needed, as long as A = `$01` at boot (`gb_init` already does this)

- `10` STOP, and reads/writes of `$FF00`, `$FF4D`, `$FF4F`, `$FF68`/`$FF69` and reads of `$FFFF`.
  These only run on CGB, and the ROM picks the path from A at reset.
- HALT, EI, RETI, RST and interrupts are never used.

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

## 4. Knowing when it's finished

The serial output will be `06-ld r,r\n\n\nPassed\n` or, on failure, the failing opcodes and then
`Failed`. Afterwards the ROM sits in an infinite `jr` loop, so the main loop should stop once it
sees `Passed`/`Failed` on serial, or after a cycle cap. The startup also runs `delay_msec 250`,
which is roughly 260k M-cycles of looping before the test starts.
