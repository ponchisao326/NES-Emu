//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include <stdlib.h>

#include "core/ppu.h"

#include "cartridge.h"
#include "core/nes.h"

const u8 ppu_colors[64][3] = {
    { 84,  84,  84}, {  0,  30, 116}, {  8,  16, 144}, { 48,   0, 136},
    { 68,   0, 100}, { 92,   0,  48}, { 84,   4,   0}, { 60,  24,   0},
    { 32,  42,   0}, {  8,  58,   0}, {  0,  64,   0}, {  0,  60,   0},
    {  0,  50,  60}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {152, 150, 152}, {  8,  76, 196}, { 48,  50, 236}, { 92,  30, 228},
    {136,  20, 176}, {160,  20, 100}, {152,  34,  32}, {120,  60,   0},
    { 84,  90,   0}, { 40, 114,   0}, {  8, 124,   0}, {  0, 118,  40},
    {  0, 102, 120}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, { 76, 154, 236}, {120, 124, 236}, {176,  98, 236},
    {228,  84, 236}, {236,  88, 180}, {236, 106, 100}, {212, 136,  32},
    {160, 170,   0}, {116, 196,   0}, { 76, 208,  32}, { 56, 204, 108},
    { 56, 180, 204}, { 60,  60,  60}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
    {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
    {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
    {160, 214, 228}, {160, 162, 160}, {  0,   0,   0}, {  0,   0,   0},
};

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
    (void)readonly;
    u8 data = 0x00;
    address &= 0x3FFF;

    if (nes->cart && cart_ppu_read(nes->cart, address, &data))
        return data;

    if (address <= 0x1FFF) {
        // Local pattern tables if cartridge didn't claim it
    } else if (address <= 0x3EFF) {
        // nametables
    } else {
        // palette
    }

    return data;
}

void ppu_write(nes_t *nes, u16 address, u8 data) {
    address &= 0x3FFF;

    if (nes->cart && cart_ppu_write(nes->cart, address, data))
        return;

    if (address <= 0x1FFF) {
        // Pattern tables only with CHR RAM
    } else if (address <= 0x3EFF) {
        // nametables
    } else {
        // palette
    }
}

void ppu_clock(nes_t *nes) {
    // Testing noise while there's no real render
    s16 x = nes->ppu.cycle - 1;
    s16 y = nes->ppu.scanline;
    if (x >= 0 && x < PPU_SCREEN_W && y >= 0 && y < PPU_SCREEN_H)
        nes->ppu.screen[y][x] = (rand() % 2) ? 0x3F : 0x30;

    nes->ppu.cycle++;
    if (nes->ppu.cycle >= 341) {
        nes->ppu.cycle = 0;
        nes->ppu.scanline++;
        if (nes->ppu.scanline >= 261) {
            nes->ppu.scanline = -1;
            nes->ppu.frame_complete = true;
        }
    }
}

void ppu_reset(nes_t *nes) {
    nes->ppu.scanline = 0;
    nes->ppu.cycle = 0;
    nes->ppu.frame_complete = false;
}