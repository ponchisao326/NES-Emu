#pragma once
#include "common/types.h"

typedef struct nes nes_t;

#define APU_CPU_RATE 1789773.0
#define APU_BUFFER_SIZE 8192

enum {
    APU_PULSE1 = 0x01,
    APU_PULSE2 = 0x02,
    APU_TRIANGLE = 0x04,
    APU_NOISE = 0x08,
    APU_DMC = 0x10
};

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
    apu_envelope_t envelope;
    apu_length_t length;
    bool mode;
    u16 shift;
    u16 period;
    u16 timer;
} apu_noise_t;

typedef struct {
    bool irq_enabled;
    bool loop;
    bool irq;
    u16 rate;
    u16 timer;
    u8 level;
    u16 sample_address;
    u16 sample_length;
    u16 current_address;
    u16 bytes_remaining;
    u8 buffer;
    bool buffer_full;
    u8 shift;
    u8 bits_remaining;
    bool silence;
    u8 stall;
} apu_dmc_t;

typedef struct {
    apu_pulse_t pulse[2];
    apu_triangle_t triangle;
    apu_noise_t noise;
    apu_dmc_t dmc;

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
    u8 out_noise;
    u8 out_dmc;

    u8 mute;
    double sample_rate;
    double sample_clock;
    double sum;
    u32 sum_count;
    double hp90_a, hp90_x, hp90_y;
    double hp440_a, hp440_x, hp440_y;
    double lp14k_a, lp14k_y;
    float samples[APU_BUFFER_SIZE];
    u32 sample_count;
} apu_t;

void apu_reset(nes_t *nes);
void apu_clock(nes_t *nes);
void apu_cpu_write(nes_t *nes, u16 address, u8 data);
u8 apu_cpu_read(nes_t *nes, u16 address, bool readonly);
bool apu_irq(nes_t *nes);
bool apu_cpu_stalled(nes_t *nes);
void apu_set_sample_rate(nes_t *nes, u32 rate);
u32 apu_take_samples(nes_t *nes, float *out, u32 max);
float apu_mix(const apu_t *apu);
