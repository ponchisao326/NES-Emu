//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#pragma once
#include "common/types.h"
#include "core/cpu6502.h"

#define BUS_FLAT_MEM

typedef struct nes {
    cpu6502_t cpu;
#ifdef BUS_FLAT_MEM
    u8 flat_ram[64 * 1024];
#else
    u8 cpu_ram[2048];
#endif
    u64 system_clock;
} nes_t;

void nes_reset(nes_t *nes);