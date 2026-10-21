//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef enum {
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    MIRROR_ONESCREEN_LO,
    MIRROR_ONESCREEN_HI
} mirror_t;

typedef struct mapper mapper_t;

struct mapper {
    bool (*cpu_map_read) (mapper_t *m, u16 address, u32 *mapped);
    bool (*cpu_map_write)(mapper_t *m, u16 address, u32 *mapped, u8 data);
    bool (*ppu_map_read) (mapper_t *m, u16 address, u32 *mapped);
    bool (*ppu_map_write)(mapper_t *m, u16 address, u32 *mapped);
    void (*scanline)(mapper_t *m);

    u8 prg_banks;
    u8 chr_banks;

    bool drives_mirroring;
    mirror_t mirror;
    bool prg_ram_enabled;
    bool prg_ram_writable;
    bool irq;

    void *state; // self status from every manager
};

void mapper000_init(mapper_t *m, u8 prg_banks, u8 chr_banks);
bool mapper001_init(mapper_t *m, u8 prg_banks, u8 chr_banks);
bool mapper002_init(mapper_t *m, u8 prg_banks, u8 chr_banks);
bool mapper004_init(mapper_t *m, u8 prg_banks, u8 chr_banks);