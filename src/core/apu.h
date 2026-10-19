#pragma once
#include "common/types.h"

typedef struct nes nes_t;

typedef struct {
    u64 cycle;
    u32 frame_step;
    bool frame_mode;
    bool frame_irq_inhibit;
    bool frame_irq;
    u8 frame_reset_delay;
    bool quarter_frame;
    bool half_frame;
} apu_t;

void apu_reset(nes_t *nes);
void apu_clock(nes_t *nes);
void apu_cpu_write(nes_t *nes, u16 address, u8 data);
u8 apu_cpu_read(nes_t *nes, u16 address, bool readonly);
bool apu_irq(nes_t *nes);
