//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef struct nes nes_t;

#define PPU_SCREEN_W 256
#define PPU_SCREEN_H 240

typedef union {
    struct {
        u8 unused : 5;
        u8 sprite_overflow : 1;
        u8 sprite_zero_hit : 1;
        u8 vertical_blank : 1;
    };
    u8 reg;
} ppustatus_t;

_Static_assert(sizeof(ppustatus_t) == 1, "PPUSTATUS must be 1 byte long");

typedef union {
    struct {
        u8 grayscale : 1;
        u8 render_background_left : 1;
        u8 render_sprites_left : 1;
        u8 render_background : 1;
        u8 render_sprites : 1;
        u8 enhance_red : 1;
        u8 enhance_green : 1;
        u8 enhance_blue : 1;
    };
    u8 reg;
} ppumask_t;

_Static_assert(sizeof(ppumask_t) == 1, "PPUMASK must be 1 byte long");

typedef union {
    struct {
        u8 nametable_x : 1;
        u8 nametable_y : 1;
        u8 increment_mode : 1;
        u8 pattern_sprite : 1;
        u8 pattern_background : 1;
        u8 sprite_size : 1;
        u8 slave_mode : 1; // unused
        u8 enable_nmi : 1;
    };
    u8 reg;
} ppuctrl_t;

_Static_assert(sizeof(ppuctrl_t) == 1, "PPUCTRL must be 1 byte long");

typedef union {
    struct {
        u16 coarse_x : 5;
        u16 coarse_y : 5;
        u16 nametable_x : 1;
        u16 nametable_y : 1;
        u16 fine_y : 3;
        u16 unused : 1;
    };
    u16 reg;
} loopy_t;

_Static_assert(sizeof(loopy_t) == 2, "loopy register must be 2 bytes long");

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

    ppustatus_t status;
    ppumask_t mask;
    ppuctrl_t ctrl;

    u8 address_latch;
    u8 data_buffer;

    loopy_t vram_addr;
    loopy_t tram_addr;
    u8 fine_x;

    u8 bg_next_tile_id;
    u8 bg_next_tile_attrib;
    u8 bg_next_tile_lsb;
    u8 bg_next_tile_msb;

    u16 bg_shifter_pattern_lo;
    u16 bg_shifter_pattern_hi;
    u16 bg_shifter_attrib_lo;
    u16 bg_shifter_attrib_hi;

    bool nmi;
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