//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/nes.h"
#include "core/bus.h"

void nes_reset(nes_t *nes) {
    cpu_reset(nes);
    nes->system_clock = 0;
}