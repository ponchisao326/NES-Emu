//
// Created by Victor Gomez Ponce on 20/09/2026.
#pragma once
#include "common/types.h"

typedef struct nes nes_t;

typedef struct {
    const char *name;
    u8 (*operate) (nes_t *nes);
    u8 (*addrmode)(nes_t *nes);
    u8 cycles;
} instruction_t;

extern const instruction_t cpu_lookup[256];