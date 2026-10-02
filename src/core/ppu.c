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
    u8 data = 0x00;

    switch (address) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            data = (nes->ppu.status.reg & 0xE0) | (nes->ppu.data_buffer & 0x1F);
            nes->ppu.status.vertical_blank = 0;
            nes->ppu.address_latch = 0;
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
            data = nes->ppu.data_buffer;
            nes->ppu.data_buffer = ppu_read(nes, nes->ppu.vram_address, readonly);

            if (nes->ppu.vram_address >= 0x3F00) data = nes->ppu.data_buffer;

            nes->ppu.vram_address += nes->ppu.ctrl.increment_mode ? 32 : 1;
            break;
    }

    return data;
}

void ppu_cpu_write(nes_t *nes, u16 address, u8 data) {
    switch (address) {
        case 0x0000: // Control
            nes->ppu.ctrl.reg = data;
            break;
        case 0x0001: // Mask
            nes->ppu.mask.reg = data;
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
            if (nes->ppu.address_latch == 0) {
                // First write
                nes->ppu.vram_address = (nes->ppu.vram_address & 0x00FF) | ((u16)data << 8);
                nes->ppu.address_latch = 1;
            } else {
                // Second write
                nes->ppu.vram_address = (nes->ppu.vram_address & 0xFF00) | data;
                nes->ppu.address_latch = 0;
            }
            break;
        case 0x0007: // PPU Data
            ppu_write(nes, nes->ppu.vram_address, data);
            nes->ppu.vram_address += nes->ppu.ctrl.increment_mode ? 32 : 1;
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
        nes->ppu.pattern_table[(address & 0x1000) >> 12][address & 0x0FFF] = data;
    } else if (address <= 0x3EFF) {
        // nametables
    } else {
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        data = nes->ppu.palette[address];
    }

    return data;
}

void ppu_write(nes_t *nes, u16 address, u8 data) {
    address &= 0x3FFF;

    if (nes->cart && cart_ppu_write(nes->cart, address, data))
        return;

    if (address <= 0x1FFF) {
        // Pattern tables only with CHR RAM
        nes->ppu.pattern_table[(address & 0x1000) >> 12][address & 0x0FFF] = data;
    } else if (address <= 0x3EFF) {
        // nametables
    } else {
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        nes->ppu.palette[address] = data;
    }
}

void ppu_clock(nes_t *nes) {
    if (nes->ppu.scanline == -1 && nes->ppu.cycle == 1) {
        nes->ppu.status.vertical_blank = 0;
    }

    if (nes->ppu.scanline == 241 && nes->ppu.cycle == 1) {
        nes->ppu.status.vertical_blank = 1;
        if (nes->ppu.ctrl.enable_nmi)
            nes->ppu.nmi = true;
    }

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

u8 ppu_colour_from_palette(nes_t *nes, u8 palette, u8 pixel) {
    return ppu_read(nes, 0x3F00 + (palette << 2) + pixel, false) & 0x3F;
}

void ppu_render_pattern_table(nes_t *nes, u8 index, u8 palette) {
    for (u16 tile_y = 0; tile_y < 16; tile_y++) {
        for (u16 tile_x = 0; tile_x < 16; tile_x++) {
            const u16 offset = tile_y * 256 + tile_x * 16;

            for (u16 row = 0; row < 8; row++) {
                u8 lsb = ppu_read(nes, index * 0x1000 + offset + row + 0x0000, false);
                u8 msb = ppu_read(nes, index * 0x1000 + offset + row + 0x0008, false);

                for (u16 col = 0; col < 8; col++) {
                    const u8 pixel = ((msb & 0x01) << 1) | (lsb & 0x01);
                    lsb >>= 1;
                    msb >>= 1;

                    nes->ppu.pattern_screen[index][tile_y * 8 + row][tile_x * 8 + (7 - col)] =
                        ppu_colour_from_palette(nes, palette, pixel);
                }
            }
        }
    }
}