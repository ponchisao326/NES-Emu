//
// Created by Victor Gomez Ponce on 14/10/2026.
//
#include <stdlib.h>

#include "core/mappers/mapper.h"

typedef struct {
    u8 bank;
} m002_state_t;

static bool m002_cpu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address < 0x8000) return false;
    const m002_state_t *state = m->state;
    const u32 bank = address < 0xC000 ? state->bank : (u32)(m->prg_banks - 1);
    *mapped = bank * 0x4000 + (address & 0x3FFF);
    return true;
}

static bool m002_cpu_map_write(mapper_t *m, u16 address, u32 *mapped, u8 data) {
    (void)mapped;
    if (address < 0x8000) return false;
    m002_state_t *state = m->state;
    state->bank = data % m->prg_banks;
    return false;
}

static bool m002_ppu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    (void)m;
    if (address <= 0x1FFF) {
        *mapped = address;
        return true;
    }
    return false;
}

static bool m002_ppu_map_write(mapper_t *m, u16 address, u32 *mapped) {
    if (address <= 0x1FFF && m->chr_banks == 0) {
        *mapped = address;
        return true;
    }
    return false;
}

bool mapper002_init(mapper_t *m, u8 prg_banks, u8 chr_banks) {
    m002_state_t *state = calloc(1, sizeof(m002_state_t));
    if (!state) return false;

    m->cpu_map_read = m002_cpu_map_read;
    m->cpu_map_write = m002_cpu_map_write;
    m->ppu_map_read = m002_ppu_map_read;
    m->ppu_map_write = m002_ppu_map_write;

    m->prg_banks = prg_banks;
    m->chr_banks = chr_banks;
    m->state = state;
    return true;
}
