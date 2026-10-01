//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include "core/ppu.h"
#include "core/nes.h"

u8 ppu_cpu_read(nes_t *nes, u16 address, bool readonly) {
    (void)nes; (void)readonly;
    u8 data = 0x00;

    switch (address) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: // PPU Data
            break;
    }

    return data;
}

void ppu_cpu_write(nes_t *nes, u16 address, u8 data) {
    (void)nes; (void)data;

    switch (address) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: // PPU Data
            break;
    }
}

u8 ppu_read(nes_t *nes, u16 address, bool readonly) {
    (void)nes; (void)readonly;
    u8 data = 0x00;
    address &= 0x3FFF;

    return data;
}

void ppu_write(nes_t *nes, u16 address, u8 data) {
    (void)nes; (void)data;
    address &= 0x3FFF;
    (void)address;
}

void ppu_clock(nes_t *nes) {
    (void)nes;
}

void ppu_reset(nes_t *nes) {
    (void)nes;
}