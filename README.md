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

- cpu_instrs
  - [x] 06-ld r,r
  - [x] 05-op rp
  - [x] 04-op r,imm
  - [x] 03-op sp,hl
  - [x] 07-jr,jp,call,ret,rst
  - [x] 08-misc instrs
  - [x] 09-op r,r
  - [x] 11-op a,(hl)
  - [x] 10-bit ops
  - [x] 01-special
  - [x] 02-interrupts
- instr_timing
- halt_bug
- mem_timing
- mem_timing2
- dmg_sound
