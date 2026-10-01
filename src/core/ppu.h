//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef struct nes nes_t;

#define PPU_SCREEN_W 256
#define PPU_SCREEN_H 240

typedef struct {
    u8 name_table[2][1024];
    u8 pattern_table[2][4096]; // Only used with CHR RAM
    u8 palette[32];

    // Framebuffer: palette index per pixel
    u8 screen[PPU_SCREEN_H][PPU_SCREEN_W];
    u8 pattern_screen[2][128][128];

    s16 scanline;
    s16 cycle;
    bool frame_complete;
} ppu_t;

// 64 fixed colours from 2C02 in RGB
extern const u8 ppu_colors[64][3];

void ppu_clock(nes_t *nes);
void ppu_reset(nes_t *nes);

// PPU registries seen from CPU ($2000- $2007)
u8 ppu_cpu_read(nes_t *nes, u16 address, bool readonly);
void ppu_cpu_write(nes_t *nes, u16 address, u8 data);

// Self-bus from PPU (patterns, name tables and palette)
u8 ppu_read(nes_t *nes, u16 address, bool readonly);
void ppu_write(nes_t *nes, u16 address, u8 data);

u8 ppu_colour_from_palette(nes_t *nes, u8 palette, u8 pixel);
void ppu_render_pattern_table(nes_t *nes, u8 index, u8 palette);