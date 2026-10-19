//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/nes.h"
#include "core/bus.h"
#include "core/apu.h"

void nes_insert_cartridge(nes_t *nes, cartridge_t *cart) {
    nes->cart = cart;
}

void nes_reset(nes_t *nes) {
    cpu_reset(nes);
    ppu_reset(nes);
    apu_reset(nes);
    nes->system_clock = 0;
    nes->dma_page = 0x00;
    nes->dma_addr = 0x00;
    nes->dma_data = 0x00;
    nes->dma_dummy = true;
    nes->dma_transfer = false;
}

static void dma_clock(nes_t *nes) {
    if (nes->dma_dummy) {
        if (nes->system_clock % 2 == 1) nes->dma_dummy = false;
        return;
    }

    if (nes->system_clock % 2 == 0) {
        nes->dma_data = bus_cpu_read(nes, (u16)(nes->dma_page << 8 | nes->dma_addr), false);
        return;
    }

    nes->ppu.oam_bytes[nes->dma_addr++] = nes->dma_data;
    if (nes->dma_addr == 0x00) {
        nes->dma_transfer = false;
        nes->dma_dummy = true;
    }
}

void nes_clock(nes_t *nes) {
    ppu_clock(nes);

    if (nes->system_clock % 3 == 0) {
        apu_clock(nes);
        if (nes->dma_transfer) {
            dma_clock(nes);
        } else if (!apu_cpu_stalled(nes)) {
            if (nes->ppu.nmi && cpu_complete(nes)) {
                nes->ppu.nmi = false;
                cpu_nmi(nes);
            } else if (apu_irq(nes) && cpu_complete(nes)) {
                cpu_irq(nes);
            }
            cpu_clock(nes);
        }
    }

    nes->system_clock++;
}