//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include "core/mappers/mapper.h"

// NROM: No commutable banks. PRG are 16 or 32 KB

static bool m000_cpu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address >= 0x8000) {
        // With 1 bank (16KB) the high half is mirrored from low one
        *mapped = address & (m->prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

static bool m000_cpu_map_write(mapper_t *m, u16 address, u32 *mapped) {
    if (address >= 0x8000) {
        *mapped = address & (m->prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }
    return false;
}

static bool m000_ppu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    (void)m;
    if (address <= 0x1FFF) {
        *mapped = address;
        return true;
    }
    return false;
}

static bool m000_ppu_map_write(mapper_t *m, u16 address, u32 *mapped) {
    if (address <= 0x1FFF && m->chr_banks == 0) {
        *mapped = address; // CHR RAM: writable
        return true;
    }
    return false;
}

void mapper000_init(mapper_t *m, u8 prg_banks, u8 chr_banks) {
    m->cpu_map_read = m000_cpu_map_read;
    m->cpu_map_write = m000_cpu_map_write;
    m->ppu_map_read = m000_ppu_map_read;
    m->ppu_map_write = m000_ppu_map_write;

    m->prg_banks = prg_banks;
    m->chr_banks = chr_banks;
    m->state = NULL; // NROM doesn't have any state
}