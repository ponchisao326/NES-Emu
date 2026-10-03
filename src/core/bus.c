//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/bus.h"

#include "cartridge.h"
#include "core/nes.h"
#include "core/ppu.h"

u8 bus_cpu_read(nes_t *nes, u16 address, bool readonly) {
    u8 data = 0x00;

    if (cart_cpu_read(nes->cart, address, &data))
        return data;

    if (address <= 0x1FFF)
        return nes->cpu_ram[address & 0x07FF];
    if (address >= 0x2000 && address <= 0x3FFF)
        return ppu_cpu_read(nes, address & 0x0007, readonly);
    if (address >= 0x4016 && address <= 0x4017) {
        // Every read returns a button and shifts the registry
        data = (nes->controller_state[address & 0x0001] & 0x80) != 0;
        if (!readonly) nes->controller_state[address & 0x0001] <<= 1;
        return data;
    }

    return 0x00;
}

void bus_cpu_write(nes_t *nes, u16 address, u8 data) {
    if (cart_cpu_write(nes->cart, address, data))
        return;

    if (address <= 0x1FFF)
        nes->cpu_ram[address & 0x07FF] = data;
    else if (address >= 0x2000 && address <= 0x3FFF)
        ppu_cpu_write(nes, address & 0x0007, data);
    else if (address >= 0x4016 && address <= 0x4017)
        // Freezes button states to start reading it
        nes->controller_state[address & 0x0001] = nes->controller[address & 0x0001];
}