#include <stdlib.h>

#include "core/mappers/mapper.h"

typedef struct {
    u8 bank_select;
    u8 bank[8];
    u8 irq_latch;
    u8 irq_counter;
    bool irq_reload;
    bool irq_enabled;
} m004_state_t;

static u32 prg_8k_banks(const mapper_t *m) {
    return (u32)m->prg_banks * 2;
}

static u32 chr_1k_banks(const mapper_t *m) {
    return m->chr_banks ? (u32)m->chr_banks * 8 : 8;
}

static u32 prg_bank_for(const mapper_t *m, const m004_state_t *state, u16 address) {
    const u32 last = prg_8k_banks(m) - 1;
    const bool swapped = state->bank_select & 0x40;

    switch ((address >> 13) & 0x03) {
        case 0: return swapped ? last - 1 : state->bank[6];
        case 1: return state->bank[7];
        case 2: return swapped ? state->bank[6] : last - 1;
        default: return last;
    }
}

static u32 chr_bank_for(const m004_state_t *state, u16 address) {
    if (state->bank_select & 0x80) address ^= 0x1000;
    const u8 slot = (address >> 10) & 0x07;
    if (slot < 4) return (state->bank[slot >> 1] & 0xFE) | (slot & 0x01);
    return state->bank[slot - 2];
}

static bool m004_cpu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address < 0x8000) return false;
    const u32 bank = prg_bank_for(m, m->state, address) % prg_8k_banks(m);
    *mapped = bank * 0x2000 + (address & 0x1FFF);
    return true;
}

static bool m004_cpu_map_write(mapper_t *m, u16 address, u32 *mapped, u8 data) {
    (void)mapped;
    if (address < 0x8000) return false;
    m004_state_t *state = m->state;

    switch (address & 0xE001) {
        case 0x8000:
            state->bank_select = data;
            break;
        case 0x8001:
            state->bank[state->bank_select & 0x07] = data;
            break;
        case 0xA000:
            m->mirror = (data & 0x01) ? MIRROR_HORIZONTAL : MIRROR_VERTICAL;
            break;
        case 0xA001:
            m->prg_ram_enabled = (data & 0x80) != 0;
            m->prg_ram_writable = (data & 0xC0) == 0x80;
            break;
        case 0xC000:
            state->irq_latch = data;
            break;
        case 0xC001:
            state->irq_counter = 0;
            state->irq_reload = true;
            break;
        case 0xE000:
            state->irq_enabled = false;
            m->irq = false;
            break;
        case 0xE001:
            state->irq_enabled = true;
            break;
    }
    return false;
}

static bool m004_ppu_map_read(mapper_t *m, u16 address, u32 *mapped) {
    if (address > 0x1FFF) return false;
    const u32 bank = chr_bank_for(m->state, address) % chr_1k_banks(m);
    *mapped = bank * 0x0400 + (address & 0x03FF);
    return true;
}

static bool m004_ppu_map_write(mapper_t *m, u16 address, u32 *mapped) {
    if (m->chr_banks != 0) return false;
    return m004_ppu_map_read(m, address, mapped);
}

static void m004_scanline(mapper_t *m) {
    m004_state_t *state = m->state;

    if (state->irq_counter == 0 || state->irq_reload) {
        state->irq_counter = state->irq_latch;
        state->irq_reload = false;
    } else {
        state->irq_counter--;
    }

    if (state->irq_counter == 0 && state->irq_enabled) m->irq = true;
}

bool mapper004_init(mapper_t *m, u8 prg_banks, u8 chr_banks) {
    m004_state_t *state = calloc(1, sizeof(m004_state_t));
    if (!state) return false;

    const u8 power_on_banks[8] = { 0, 2, 4, 5, 6, 7, 0, 1 };
    for (int i = 0; i < 8; i++) state->bank[i] = power_on_banks[i];

    m->cpu_map_read = m004_cpu_map_read;
    m->cpu_map_write = m004_cpu_map_write;
    m->ppu_map_read = m004_ppu_map_read;
    m->ppu_map_write = m004_ppu_map_write;
    m->scanline = m004_scanline;

    m->prg_banks = prg_banks;
    m->chr_banks = chr_banks;
    m->drives_mirroring = true;
    m->mirror = MIRROR_VERTICAL;
    m->prg_ram_enabled = true;
    m->prg_ram_writable = true;
    m->irq = false;
    m->state = state;
    return true;
}
