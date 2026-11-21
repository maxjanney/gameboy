# Gameboy Emulator

A Dot Matrix Game(DMG) GameBoy emulator.

## Goals

Goals for this project

- Experiment with [unity builds](https://www.youtube.com/watch?v=jgiMagdjA1s)
- Maybe make use of [arena allocators](https://www.youtube.com/watch?v=j3BvierE2hM)
- Use SDL2 as a frontend for Desktops only
- Support most games
- DMG only, no color or super gameboy support
- Well tested against [Blargg's test ROMs](https://github.com/retrio/gb-test-roms/tree/master)
- Implement audio

## TODO

Pass gb-test-roms

- [ ] cpu_instrs
  - [x] 01-special
  - [x] 02-interrupts
  - [x] 03-op sp,hl
  - [x] 04-op r,imm
  - [x] 05-op rp
  - [x] 06-ld r,r
  - [x] 07-jr,jp,call,ret,rst
  - [x] 08-misc instrs
  - [x] 09-op r,r
  - [x] 10-bit ops
  - [x] 11-op a,(hl)
  - [ ] cpu_instrs.gb
- [x] instr_timing
- [x] halt_bug
- [ ] mem_timing
- [ ] mem_timing2
- [ ] dmg_sound

Memory Bank Controllers

- [ ] mbc1
  - [ ] possibly fix mask in gb_read_rom
  - [ ] properly set cart ram/rom size to a power of 2
  - [ ] handle banking mode/understand a little bit better
  - [ ] writing to eram (ram enable flag)
  - [ ] update banking in gb_mbc_write_rom
