//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#pragma once
#include "common/types.h"
#include "core/cpu6502.h"
#include "core/ppu.h"

typedef struct cartridge cartridge_t;

typedef struct nes {
    cpu6502_t cpu;
    ppu_t ppu;
    cartridge_t *cart;

    u8 cpu_ram[2048];
    u64 system_clock;
} nes_t;

void nes_insert_cartridge(nes_t *nes, cartridge_t *cart);
void nes_reset(nes_t *nes);
void nes_clock(nes_t *nes);