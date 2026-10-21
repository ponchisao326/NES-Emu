#include <stdlib.h>

#include "core/mappers/mapper.h"

typedef struct {
    u8 shift;
    u8 control;
    u8 chr_bank[2];
    u8 prg_bank;
} m001_state_t;

static const mirror_t control_mirroring[4] = {
    MIRROR_ONESCREEN_LO,
    MIRROR_ONESCREEN_HI,
    MIRROR_VERTICAL,
    MIRROR_HORIZONTAL
};

static u32 chr_4k_banks(const mapper_t *m) {
    return m->chr_banks ? (u32)m->chr_banks * 2 : 2;
}

static u32 prg_bank_for(const mapper_t *m, const m001_state_t *state, u16 address) {
    const u8 outer = m->prg_banks > 16 ? (state->chr_bank[0] & 0x10) : 0x00;
    const u8 inner = state->prg_bank & 0x0F;
    const bool upper_half = address >= 0xC000;

    switch ((state->control >> 2) & 0x03) {
        case 2: return upper_half ? (outer | inner) : outer;
        case 3: return upper_half ? (outer | 0x0F) : (outer | inner);
        default: return outer | (inner & 0x0E) | upper_half;
    }
}

static u32 chr_bank_for(const m001_state_t *state, u16 address) {
    const u8 half = (address >> 12) & 0x01;
    if (state->control & 0x10) return state->chr_bank[half];
    return (state->chr_bank[0] & 0x1E) | half;
}

static void write_register(mapper_t *m, m001_state_t *state, u16 address, u8 value) {
    switch ((address >> 13) & 0x03) {
        case 0:
            state->control = value;
            m->mirror = control_mirroring[value & 0x03];
            break;
        case 1:
            state->chr_bank[0] = value;
            break;
        case 2:
            state->chr_bank[1] = value;
            break;
        case 3:
            state->prg_bank = value;
            m->prg_ram_enabled = !(value & 0x10);
            m->prg_ram_writable = m->prg_ram_enabled;
            break;
    }
}

static bool m001_cpu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address < 0x8000) return false;
    const u32 bank = prg_bank_for(m, m->state, address) % m->prg_banks;
    *mapped = bank * 0x4000 + (address & 0x3FFF);
    return true;
}

static bool m001_cpu_map_write(mapper_t *m, u16 address, u32 *mapped, u8 data) {
    (void)mapped;
    if (address < 0x8000) return false;
    m001_state_t *state = m->state;

    if (data & 0x80) {
        state->shift = 0x10;
        state->control |= 0x0C;
        return false;
    }

    const bool last_bit = state->shift & 0x01;
    state->shift = (u8)((state->shift >> 1) | ((data & 0x01) << 4));

    if (last_bit) {
        write_register(m, state, address, state->shift);
        state->shift = 0x10;
    }
    return false;
}

static bool m001_ppu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address > 0x1FFF) return false;
    const u32 bank = chr_bank_for(m->state, address) % chr_4k_banks(m);
    *mapped = bank * 0x1000 + (address & 0x0FFF);
    return true;
}

static bool m001_ppu_map_write(mapper_t *m, u16 address, u32 *mapped) {
    if (m->chr_banks != 0) return false;
    return m001_ppu_map_read(m, address, mapped);
}

bool mapper001_init(mapper_t *m, u8 prg_banks, u8 chr_banks) {
    m001_state_t *state = calloc(1, sizeof(m001_state_t));
    if (!state) return false;

    state->shift = 0x10;
    state->control = 0x0C;

    m->cpu_map_read = m001_cpu_map_read;
    m->cpu_map_write = m001_cpu_map_write;
    m->ppu_map_read = m001_ppu_map_read;
    m->ppu_map_write = m001_ppu_map_write;
    m->scanline = NULL;

    m->prg_banks = prg_banks;
    m->chr_banks = chr_banks;
    m->drives_mirroring = true;
    m->mirror = control_mirroring[state->control & 0x03];
    m->prg_ram_enabled = true;
    m->prg_ram_writable = true;
    m->irq = false;
    m->state = state;
    return true;
}
