//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef struct mapper mapper_t;

struct mapper {
    bool (*cpu_map_read) (mapper_t *m, u16 address, u32 *mapped);
    bool (*cpu_map_write)(mapper_t *m, u16 address, u32 *mapped);
    bool (*ppu_map_read) (mapper_t *m, u16 address, u32 *mapped);
    bool (*ppu_map_write)(mapper_t *m, u16 address, u32 *mapped);

    u8 prg_banks;
    u8 chr_banks;
    void *state; // self status from every manager
};

void mapper000_init(mapper_t *m, u8 prg_banks, u8 chr_banks);