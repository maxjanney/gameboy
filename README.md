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

Running gb-test-roms

- cpu_instrs
  - [x] 06-ld r,r
  - [x] 05-op rp
  - [ ] 04-op r,imm
  - [ ] 03-op sp,hl
  - [ ] 07-jr,jp,call,ret,rst
  - [ ] 08-misc instrs
  - [ ] 09-op r,r
  - [ ] 11-op a,(hl)
  - [ ] 10-bit ops
  - [ ] 01-special
  - [ ] 02-interrupts
- instr_timing
- mem_timing
- mem_timing2
- halt_bug
- dmg_sound
- oam_bug
