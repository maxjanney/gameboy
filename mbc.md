# Cartridge Header

At address 0x0147, the byte tells us the 'cartridge type'. Part of that tells us the mapper  
See the pandocs for a table
https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type

0x0149 tells us the RAM size.
See the pandocs for the size table
https://gbdev.io/pandocs/The_Cartridge_Header.html#0149--ram-size

# No MBC

32 KiB of ROM only. No cart. RAM.

# MBC1

Thinking of having the mbc be a union of mbc1/3/5 structs. Along side this union we can have which type it is.

## RAM

Supports up to 32 KiB of banked RAM. The control registers are mapped to writes to parts of ROM.

### Registers

RAM Enable - mapped to 0000-1fff (write only). Any value with $A in the lower 4 bits enables the RAM attached to the MBC, and any other value disables the RAM. This can be a flag on the GB struct.

RAM Bank No - 4000-5fff (write only). 2-bit register for the RAM bank. Or to specify the upper two bits (bits 5-6) of the ROM Bank number. This can be computed at the time we write so reading from mbc is cleaner.

Banking Mode Select - 6000–7FFF, 1-bit register selects the two mbc1 banking modes.
Mode 0: default, 0000-3fff is rom bank 0. a000-bfff are locked to ram bank 0.
Mode 1: 0000-3fff (rom bank 0) and a000-bfff (eram) can be bank-switched via RAM bank no.

In both modes, the 2 bits from RAM Bank No apply to ROM bank no.

## ROM

Supports up to 512 KiB ROM.

### Registers

ROM Bank No - 2000-3fff (write only). 5-bit register. When writing the value, we can set it to 1 if its 0.
When inspecting this value, we must take the 5 bits + use upper 2 bits from ram bank no register. We must also do the masking, but I think to
make mbc reads easier, we can set the rom bank number on a field in the gb.
