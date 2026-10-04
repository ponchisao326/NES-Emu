//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/nes.h"

void nes_insert_cartridge(nes_t *nes, cartridge_t *cart) {
    nes->cart = cart;
}

void nes_reset(nes_t *nes) {
    cpu_reset(nes);
    ppu_reset(nes);
    nes->system_clock = 0;
}

void nes_clock(nes_t *nes) {
    ppu_clock(nes);

    if (nes->system_clock % 3 == 0) {
        if (nes->ppu.nmi && cpu_complete(nes)) {
            nes->ppu.nmi = false;
            cpu_nmi(nes);
        }
        cpu_clock(nes);
    }

    nes->system_clock++;
}