#include "core/apu.h"

#include <string.h>

#include "core/nes.h"

void apu_reset(nes_t *nes) {
    memset(&nes->apu, 0, sizeof nes->apu);
}

static void frame_counter_clock(apu_t *apu) {
    apu->quarter_frame = false;
    apu->half_frame = false;

    if (apu->frame_reset_delay > 0 && --apu->frame_reset_delay == 0) {
        apu->frame_step = 0;
        if (apu->frame_mode) {
            apu->quarter_frame = true;
            apu->half_frame = true;
        }
        return;
    }

    apu->frame_step++;

    if (!apu->frame_mode) {
        switch (apu->frame_step) {
            case 7457: apu->quarter_frame = true; break;
            case 14913: apu->quarter_frame = true; apu->half_frame = true; break;
            case 22371: apu->quarter_frame = true; break;
            case 29828: if (!apu->frame_irq_inhibit) apu->frame_irq = true; break;
            case 29829:
                apu->quarter_frame = true;
                apu->half_frame = true;
                if (!apu->frame_irq_inhibit) apu->frame_irq = true;
                break;
            case 29830:
                if (!apu->frame_irq_inhibit) apu->frame_irq = true;
                apu->frame_step = 0;
                break;
            default: break;
        }
    } else {
        switch (apu->frame_step) {
            case 7457: apu->quarter_frame = true; break;
            case 14913: apu->quarter_frame = true; apu->half_frame = true; break;
            case 22371: apu->quarter_frame = true; break;
            case 37281: apu->quarter_frame = true; apu->half_frame = true; break;
            case 37282: apu->frame_step = 0; break;
            default: break;
        }
    }
}

void apu_clock(nes_t *nes) {
    apu_t *apu = &nes->apu;
    frame_counter_clock(apu);
    apu->cycle++;
}

void apu_cpu_write(nes_t *nes, u16 address, u8 data) {
    apu_t *apu = &nes->apu;
    switch (address) {
        case 0x4017:
            apu->frame_mode = (data & 0x80) != 0;
            apu->frame_irq_inhibit = (data & 0x40) != 0;
            if (apu->frame_irq_inhibit) apu->frame_irq = false;
            apu->frame_reset_delay = (apu->cycle & 1) ? 4 : 3;
            break;
        default: break;
    }
}

u8 apu_cpu_read(nes_t *nes, u16 address, bool readonly) {
    apu_t *apu = &nes->apu;
    if (address != 0x4015) return 0x00;

    u8 data = 0x00;
    if (apu->frame_irq) data |= 0x40;
    if (!readonly) apu->frame_irq = false;
    return data;
}

bool apu_irq(nes_t *nes) {
    return nes->apu.frame_irq;
}
