#pragma once
#include "common/types.h"

typedef struct nes nes_t;

typedef struct {
    bool start;
    bool loop;
    bool constant;
    u8 volume;
    u8 divider;
    u8 decay;
} apu_envelope_t;

typedef struct {
    bool enabled;
    u8 counter;
} apu_length_t;

typedef struct {
    apu_envelope_t envelope;
    apu_length_t length;
    u8 duty;
    u8 step;
    u16 period;
    u16 timer;
    bool sweep_enabled;
    bool sweep_negate;
    bool sweep_reload;
    u8 sweep_period;
    u8 sweep_shift;
    u8 sweep_divider;
    bool ones_complement;
} apu_pulse_t;

typedef struct {
    apu_length_t length;
    bool control;
    bool linear_reload;
    u8 linear_period;
    u8 linear_counter;
    u8 step;
    u16 period;
    u16 timer;
} apu_triangle_t;

typedef struct {
    apu_pulse_t pulse[2];
    apu_triangle_t triangle;

    u64 cycle;
    u32 frame_step;
    bool frame_mode;
    bool frame_irq_inhibit;
    bool frame_irq;
    u8 frame_reset_delay;
    bool quarter_frame;
    bool half_frame;

    u8 out_pulse[2];
    u8 out_triangle;
} apu_t;

void apu_reset(nes_t *nes);
void apu_clock(nes_t *nes);
void apu_cpu_write(nes_t *nes, u16 address, u8 data);
u8 apu_cpu_read(nes_t *nes, u16 address, bool readonly);
bool apu_irq(nes_t *nes);
