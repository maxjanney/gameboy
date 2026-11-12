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
  - [x] 04-op r,imm
  - [x] 03-op sp,hl
  - [x] 07-jr,jp,call,ret,rst
  - [x] 08-misc instrs
  - [x] 09-op r,r
  - [x] 11-op a,(hl)
  - [x] 10-bit ops
  - [x] 01-special
  - [ ] 02-interrupts
    - [x] stop instruction (resets DIV)
    - [x] stop instruction 2 bytes!
    - [ ] stop instruction stops cpu and timers
    - [ ] inc DIV at 16384Hz when not stopped
    - [ ] inc TIMA at [freq_map](gbdev.io/pandocs/Timer_and_Divider_Registers.html#ff07--tac-timer-control)[TAC & 0x03] if (TAC & 0x04) && (!gb->stopped)
    - [ ] when TIMA overflows (TIMA==0 after inc), reset to value in gb->io_regs[GB_IO_TMA] and request timer interrupt (gb->io_regs[GB_IO_IF] |= (1 << 2))
    - [ ] figure out how to handle cycle counting? Then the above items are easy
- instr_timing
- mem_timing
- mem_timing2
- halt_bug
- dmg_sound
- oam_bug
