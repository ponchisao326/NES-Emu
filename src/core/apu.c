#include "core/apu.h"

#include <string.h>

#include "core/nes.h"
#include "core/bus.h"

static const u8 length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const u8 duty_table[4][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 1 },
    { 0, 0, 0, 0, 0, 0, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 0, 0 }
};

static const u8 triangle_table[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static const u16 noise_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const u16 dmc_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

static float pulse_mix[31];
static float tnd_mix[203];
static bool mix_ready = false;

static void mix_tables(void) {
    pulse_mix[0] = 0.0f;
    for (int n = 1; n < 31; n++) pulse_mix[n] = (float)(95.52 / (8128.0 / n + 100.0));
    tnd_mix[0] = 0.0f;
    for (int n = 1; n < 203; n++) tnd_mix[n] = (float)(163.67 / (24329.0 / n + 100.0));
    mix_ready = true;
}

void apu_reset(nes_t *nes) {
    if (!mix_ready) mix_tables();
    const u8 mute = nes->apu.mute;
    const double rate = nes->apu.sample_rate;
    memset(&nes->apu, 0, sizeof nes->apu);
    nes->apu.mute = mute;
    apu_set_sample_rate(nes, (u32)rate);
    nes->apu.pulse[0].ones_complement = true;
    nes->apu.noise.shift = 1;
    nes->apu.noise.period = noise_table[0];
    nes->apu.dmc.rate = dmc_table[0];
    nes->apu.dmc.bits_remaining = 8;
    nes->apu.dmc.silence = true;
}

static void envelope_write(apu_envelope_t *e, u8 data) {
    e->loop = (data & 0x20) != 0;
    e->constant = (data & 0x10) != 0;
    e->volume = data & 0x0F;
}

static void envelope_clock(apu_envelope_t *e) {
    if (e->start) {
        e->start = false;
        e->decay = 15;
        e->divider = e->volume;
    } else if (e->divider == 0) {
        e->divider = e->volume;
        if (e->decay > 0) e->decay--;
        else if (e->loop) e->decay = 15;
    } else {
        e->divider--;
    }
}

static u8 envelope_output(const apu_envelope_t *e) {
    return e->constant ? e->volume : e->decay;
}

static void length_load(apu_length_t *l, u8 data) {
    if (l->enabled) l->counter = length_table[data >> 3];
}

static void length_enable(apu_length_t *l, bool enabled) {
    l->enabled = enabled;
    if (!enabled) l->counter = 0;
}

static void length_clock(apu_length_t *l, bool halt) {
    if (!halt && l->counter > 0) l->counter--;
}

static u16 sweep_target(const apu_pulse_t *p) {
    const u16 change = p->period >> p->sweep_shift;
    if (!p->sweep_negate) return (u16)(p->period + change);
    const u16 sub = (u16)(change + (p->ones_complement ? 1 : 0));
    return sub > p->period ? 0 : (u16)(p->period - sub);
}

static bool pulse_muted(const apu_pulse_t *p) {
    return p->period < 8 || sweep_target(p) > 0x7FF;
}

static void sweep_clock(apu_pulse_t *p) {
    if (p->sweep_divider == 0 && p->sweep_enabled && p->sweep_shift > 0 && !pulse_muted(p))
        p->period = sweep_target(p);
    if (p->sweep_divider == 0 || p->sweep_reload) {
        p->sweep_divider = p->sweep_period;
        p->sweep_reload = false;
    } else {
        p->sweep_divider--;
    }
}

static void pulse_timer_clock(apu_pulse_t *p) {
    if (p->timer == 0) {
        p->timer = p->period;
        p->step = (u8)((p->step - 1) & 0x07);
    } else {
        p->timer--;
    }
}

static u8 pulse_output(const apu_pulse_t *p) {
    if (p->length.counter == 0 || pulse_muted(p) || !duty_table[p->duty][p->step]) return 0;
    return envelope_output(&p->envelope);
}

static void linear_clock(apu_triangle_t *t) {
    if (t->linear_reload) t->linear_counter = t->linear_period;
    else if (t->linear_counter > 0) t->linear_counter--;
    if (!t->control) t->linear_reload = false;
}

static void triangle_timer_clock(apu_triangle_t *t) {
    if (t->timer == 0) {
        t->timer = t->period;
        if (t->length.counter > 0 && t->linear_counter > 0 && t->period >= 2)
            t->step = (u8)((t->step + 1) & 0x1F);
    } else {
        t->timer--;
    }
}

static void triangle_write(apu_triangle_t *t, u16 reg, u8 data) {
    switch (reg) {
        case 0:
            t->control = (data & 0x80) != 0;
            t->linear_period = data & 0x7F;
            break;
        case 2:
            t->period = (u16)((t->period & 0x0700) | data);
            break;
        case 3:
            t->period = (u16)((t->period & 0x00FF) | ((data & 0x07) << 8));
            length_load(&t->length, data);
            t->linear_reload = true;
            break;
        default: break;
    }
}

static void noise_timer_clock(apu_noise_t *n) {
    if (n->timer == 0) {
        n->timer = (u16)(n->period - 1);
        const u16 feedback = (n->shift ^ (n->shift >> (n->mode ? 6 : 1))) & 0x0001;
        n->shift = (u16)((n->shift >> 1) | (feedback << 14));
    } else {
        n->timer--;
    }
}

static u8 noise_output(const apu_noise_t *n) {
    if (n->length.counter == 0 || (n->shift & 0x0001)) return 0;
    return envelope_output(&n->envelope);
}

static void noise_write(apu_noise_t *n, u16 reg, u8 data) {
    switch (reg) {
        case 0:
            envelope_write(&n->envelope, data);
            break;
        case 2:
            n->mode = (data & 0x80) != 0;
            n->period = noise_table[data & 0x0F];
            break;
        case 3:
            length_load(&n->length, data);
            n->envelope.start = true;
            break;
        default: break;
    }
}

static void dmc_restart(apu_dmc_t *d) {
    d->current_address = d->sample_address;
    d->bytes_remaining = d->sample_length;
}

static void dmc_reader_clock(nes_t *nes, apu_dmc_t *d) {
    if (d->buffer_full || d->bytes_remaining == 0) return;

    d->buffer = bus_cpu_read(nes, d->current_address, false);
    d->buffer_full = true;
    d->stall = 4;
    d->current_address = d->current_address == 0xFFFF ? 0x8000 : (u16)(d->current_address + 1);

    if (--d->bytes_remaining == 0) {
        if (d->loop) dmc_restart(d);
        else if (d->irq_enabled) d->irq = true;
    }
}

static void dmc_output_clock(apu_dmc_t *d) {
    if (!d->silence) {
        if (d->shift & 0x01) {
            if (d->level <= 125) d->level += 2;
        } else if (d->level >= 2) {
            d->level -= 2;
        }
    }
    d->shift >>= 1;

    if (--d->bits_remaining == 0) {
        d->bits_remaining = 8;
        if (d->buffer_full) {
            d->silence = false;
            d->shift = d->buffer;
            d->buffer_full = false;
        } else {
            d->silence = true;
        }
    }
}

static void dmc_timer_clock(apu_dmc_t *d) {
    if (d->timer == 0) {
        d->timer = (u16)(d->rate - 1);
        dmc_output_clock(d);
    } else {
        d->timer--;
    }
}

static void dmc_write(apu_dmc_t *d, u16 reg, u8 data) {
    switch (reg) {
        case 0:
            d->irq_enabled = (data & 0x80) != 0;
            d->loop = (data & 0x40) != 0;
            d->rate = dmc_table[data & 0x0F];
            if (!d->irq_enabled) d->irq = false;
            break;
        case 1:
            d->level = data & 0x7F;
            break;
        case 2:
            d->sample_address = (u16)(0xC000 | (data << 6));
            break;
        case 3:
            d->sample_length = (u16)((data << 4) | 1);
            break;
        default: break;
    }
}

static void pulse_write(apu_pulse_t *p, u16 reg, u8 data) {
    switch (reg) {
        case 0:
            p->duty = data >> 6;
            envelope_write(&p->envelope, data);
            break;
        case 1:
            p->sweep_enabled = (data & 0x80) != 0;
            p->sweep_period = (data >> 4) & 0x07;
            p->sweep_negate = (data & 0x08) != 0;
            p->sweep_shift = data & 0x07;
            p->sweep_reload = true;
            break;
        case 2:
            p->period = (u16)((p->period & 0x0700) | data);
            break;
        case 3:
            p->period = (u16)((p->period & 0x00FF) | ((data & 0x07) << 8));
            length_load(&p->length, data);
            p->step = 0;
            p->envelope.start = true;
            break;
        default: break;
    }
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

static const double TAU = 6.283185307179586;

static double high_pass_a(double cutoff, double rate) {
    const double rc = 1.0 / (TAU * cutoff);
    return rc / (rc + 1.0 / rate);
}

static double low_pass_a(double cutoff, double rate) {
    const double rc = 1.0 / (TAU * cutoff);
    const double dt = 1.0 / rate;
    return dt / (rc + dt);
}

void apu_set_sample_rate(nes_t *nes, u32 rate) {
    apu_t *apu = &nes->apu;
    apu->sample_rate = rate;
    apu->sample_clock = 0.0;
    apu->sum = 0.0;
    apu->sum_count = 0;
    apu->sample_count = 0;
    apu->hp90_x = apu->hp90_y = apu->hp440_x = apu->hp440_y = apu->lp14k_y = 0.0;
    if (rate == 0) return;
    apu->hp90_a = high_pass_a(90.0, rate);
    apu->hp440_a = high_pass_a(440.0, rate);
    apu->lp14k_a = low_pass_a(14000.0, rate);
}

float apu_mix(const apu_t *apu) {
    const u8 p1 = (apu->mute & APU_PULSE1) ? 0 : apu->out_pulse[0];
    const u8 p2 = (apu->mute & APU_PULSE2) ? 0 : apu->out_pulse[1];
    const u8 t = (apu->mute & APU_TRIANGLE) ? 0 : apu->out_triangle;
    const u8 n = (apu->mute & APU_NOISE) ? 0 : apu->out_noise;
    const u8 d = (apu->mute & APU_DMC) ? 0 : apu->out_dmc;
    return pulse_mix[p1 + p2] + tnd_mix[3 * t + 2 * n + d];
}

static void push_sample(apu_t *apu, double x) {
    const double hp90 = apu->hp90_a * (apu->hp90_y + x - apu->hp90_x);
    apu->hp90_x = x;
    apu->hp90_y = hp90;

    const double hp440 = apu->hp440_a * (apu->hp440_y + hp90 - apu->hp440_x);
    apu->hp440_x = hp90;
    apu->hp440_y = hp440;

    apu->lp14k_y += apu->lp14k_a * (hp440 - apu->lp14k_y);

    if (apu->sample_count < APU_BUFFER_SIZE) apu->samples[apu->sample_count++] = (float)apu->lp14k_y;
}

static void resample(apu_t *apu) {
    if (apu->sample_rate == 0.0) return;
    apu->sum += apu_mix(apu);
    apu->sum_count++;
    apu->sample_clock += apu->sample_rate;
    if (apu->sample_clock >= APU_CPU_RATE) {
        apu->sample_clock -= APU_CPU_RATE;
        push_sample(apu, apu->sum / apu->sum_count);
        apu->sum = 0.0;
        apu->sum_count = 0;
    }
}

u32 apu_take_samples(nes_t *nes, float *out, u32 max) {
    apu_t *apu = &nes->apu;
    const u32 n = apu->sample_count < max ? apu->sample_count : max;
    memcpy(out, apu->samples, n * sizeof(float));
    memmove(apu->samples, apu->samples + n, (apu->sample_count - n) * sizeof(float));
    apu->sample_count -= n;
    return n;
}

void apu_clock(nes_t *nes) {
    apu_t *apu = &nes->apu;
    frame_counter_clock(apu);

    if (apu->quarter_frame) {
        envelope_clock(&apu->pulse[0].envelope);
        envelope_clock(&apu->pulse[1].envelope);
        envelope_clock(&apu->noise.envelope);
        linear_clock(&apu->triangle);
    }
    if (apu->half_frame) {
        for (int i = 0; i < 2; i++) {
            length_clock(&apu->pulse[i].length, apu->pulse[i].envelope.loop);
            sweep_clock(&apu->pulse[i]);
        }
        length_clock(&apu->triangle.length, apu->triangle.control);
        length_clock(&apu->noise.length, apu->noise.envelope.loop);
    }

    triangle_timer_clock(&apu->triangle);

    if (apu->cycle & 1) {
        pulse_timer_clock(&apu->pulse[0]);
        pulse_timer_clock(&apu->pulse[1]);
    }
    noise_timer_clock(&apu->noise);
    dmc_reader_clock(nes, &apu->dmc);
    dmc_timer_clock(&apu->dmc);

    apu->out_pulse[0] = pulse_output(&apu->pulse[0]);
    apu->out_pulse[1] = pulse_output(&apu->pulse[1]);
    apu->out_triangle = triangle_table[apu->triangle.step];
    apu->out_noise = noise_output(&apu->noise);
    apu->out_dmc = apu->dmc.level;

    resample(apu);
    apu->cycle++;
}

void apu_cpu_write(nes_t *nes, u16 address, u8 data) {
    apu_t *apu = &nes->apu;
    if (address <= 0x4007) {
        pulse_write(&apu->pulse[(address >> 2) & 1], address & 0x03, data);
        return;
    }
    if (address <= 0x400B) {
        triangle_write(&apu->triangle, address & 0x03, data);
        return;
    }
    if (address <= 0x400F) {
        noise_write(&apu->noise, address & 0x03, data);
        return;
    }
    if (address <= 0x4013) {
        dmc_write(&apu->dmc, address & 0x03, data);
        return;
    }
    switch (address) {
        case 0x4015:
            length_enable(&apu->pulse[0].length, (data & 0x01) != 0);
            length_enable(&apu->pulse[1].length, (data & 0x02) != 0);
            length_enable(&apu->triangle.length, (data & 0x04) != 0);
            length_enable(&apu->noise.length, (data & 0x08) != 0);
            if (!(data & 0x10)) apu->dmc.bytes_remaining = 0;
            else if (apu->dmc.bytes_remaining == 0) dmc_restart(&apu->dmc);
            apu->dmc.irq = false;
            break;
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
    if (apu->pulse[0].length.counter > 0) data |= 0x01;
    if (apu->pulse[1].length.counter > 0) data |= 0x02;
    if (apu->triangle.length.counter > 0) data |= 0x04;
    if (apu->noise.length.counter > 0) data |= 0x08;
    if (apu->dmc.bytes_remaining > 0) data |= 0x10;
    if (apu->frame_irq) data |= 0x40;
    if (apu->dmc.irq) data |= 0x80;
    if (!readonly) apu->frame_irq = false;
    return data;
}

bool apu_irq(nes_t *nes) {
    return nes->apu.frame_irq || nes->apu.dmc.irq;
}

bool apu_cpu_stalled(nes_t *nes) {
    if (nes->apu.dmc.stall == 0) return false;
    nes->apu.dmc.stall--;
    return true;
}
