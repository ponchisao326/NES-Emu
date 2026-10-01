//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/bus.h"
#include "core/nes.h"
#include "core/ppu.h"

u8 bus_cpu_read(nes_t *nes, u16 address, bool readonly) {
    (void)readonly;
    if (address <= 0x1FFF)
        return nes->cpu_ram[address & 0x07FF]; // mirrored 2KB until $1FFF
    if (address >= 0x2000 && address <= 0x3FFF) return ppu_cpu_read(nes, address & 0x0007, readonly);
    return 0x00;
}

void bus_cpu_write(nes_t *nes, u16 address, u8 data) {
    if (address <= 0x1FFF)
        nes->cpu_ram[address & 0x07FF] = data;
    else if (address >= 0x2000 && address <= 0x3FFF)
        ppu_cpu_write(nes, address & 0x0007, data);
}