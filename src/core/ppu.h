//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef struct nes nes_t;

typedef struct {
    u8 name_table[2][1024];
    u8 palette[32];
    
    s16 scanline;
    s16 cycle;
    bool frame_complete;
} ppu_t;

void ppu_clock(nes_t *nes);
void ppu_reset(nes_t *nes);

// PPU registries seen from CPU ($2000- $2007)
u8 ppu_cpu_read(nes_t *nes, u16 address, bool readonly);
void ppu_cpu_write(nes_t *nes, u16 address, u8 data);

// Self-bus from PPU (patterns, name tables and palette)
u8 ppu_read(nes_t *nes, u16 address, bool readonly);
void ppu_write(nes_t *nes, u16 address, u8 data);