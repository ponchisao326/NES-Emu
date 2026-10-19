#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/nes.h"
#include "core/apu.h"
#include "core/cartridge.h"

#define RATE 48000

static nes_t nes;

static void put16(FILE *f, u16 v) {
    fputc(v & 0xFF, f);
    fputc(v >> 8, f);
}

static void put32(FILE *f, u32 v) {
    put16(f, (u16)(v & 0xFFFF));
    put16(f, (u16)(v >> 16));
}

static void header(FILE *f, u32 samples) {
    fwrite("RIFF", 1, 4, f);
    put32(f, 36 + samples * 2);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16);
    put16(f, 1);
    put16(f, 1);
    put32(f, RATE);
    put32(f, RATE * 2);
    put16(f, 2);
    put16(f, 16);
    fwrite("data", 1, 4, f);
    put32(f, samples * 2);
}

static u8 channel_mask(const char *name) {
    if (strcmp(name, "pulse1") == 0) return APU_PULSE1;
    if (strcmp(name, "pulse2") == 0) return APU_PULSE2;
    if (strcmp(name, "triangle") == 0) return APU_TRIANGLE;
    if (strcmp(name, "noise") == 0) return APU_NOISE;
    if (strcmp(name, "dmc") == 0) return APU_DMC;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <rom> <frames> <out.wav> [--start F,F...] [--solo pulse1|pulse2|triangle|noise|dmc] [--gain G]\n", argv[0]);
        return 2;
    }
    const int frames = atoi(argv[2]);
    const char *out = argv[3];
    int starts[64];
    int start_count = 0;
    u8 solo = 0;
    double gain = 1.0;

    for (int i = 4; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--start") == 0) {
            char *list = argv[i + 1];
            for (char *tok = strtok(list, ","); tok && start_count < 64; tok = strtok(NULL, ","))
                starts[start_count++] = atoi(tok);
        } else if (strcmp(argv[i], "--solo") == 0) {
            solo = channel_mask(argv[i + 1]);
            if (!solo) {
                fprintf(stderr, "unknown channel %s\n", argv[i + 1]);
                return 2;
            }
        } else if (strcmp(argv[i], "--gain") == 0) {
            gain = atof(argv[i + 1]);
        }
    }

    cartridge_t *cart = cartridge_load(argv[1]);
    if (!cart) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    nes_insert_cartridge(&nes, cart);
    nes.apu.mute = solo ? (u8)(0x1F & ~solo) : 0;
    nes.apu.sample_rate = RATE;
    nes_reset(&nes);

    FILE *f = fopen(out, "wb");
    if (!f) {
        fprintf(stderr, "cannot write %s\n", out);
        return 1;
    }
    header(f, 0);

    static float buffer[APU_BUFFER_SIZE];
    u32 total = 0;
    float peak = 0.0f;
    for (int frame = 0; frame < frames; frame++) {
        nes.controller[0] = 0x00;
        for (int i = 0; i < start_count; i++)
            if (frame >= starts[i] && frame < starts[i] + 4) nes.controller[0] = 0x10;
        do { nes_clock(&nes); } while (!nes.ppu.frame_complete);
        nes.ppu.frame_complete = false;

        const u32 n = apu_take_samples(&nes, buffer, APU_BUFFER_SIZE);
        for (u32 i = 0; i < n; i++) {
            const double v = buffer[i] * gain;
            if ((float)(v < 0 ? -v : v) > peak) peak = (float)(v < 0 ? -v : v);
            const double c = v > 1.0 ? 1.0 : (v < -1.0 ? -1.0 : v);
            put16(f, (u16)(s16)(c * 32767.0));
        }
        total += n;
    }

    fseek(f, 0, SEEK_SET);
    header(f, total);
    fclose(f);
    printf("%s: %d frames, %u samples at %d Hz (%.2f s), peak %.3f\n", out, frames, total, RATE, total / (double)RATE, peak);
    cartridge_free(cart);
    return 0;
}
