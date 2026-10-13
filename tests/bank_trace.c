//
// Created by Victor Gomez Ponce on 14/10/2026.
//
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/nes.h"
#include "core/bus.h"
#include "core/cartridge.h"
#include "core/cpu6502.h"
#include "core/cpu6502_tables.h"

static u8 peek(nes_t *nes, u16 address) {
    return bus_cpu_read(nes, address, true);
}

static int window_bank(cartridge_t *cart, u16 address) {
    u32 mapped = 0;
    if (!cart->mapper.cpu_map_read(&cart->mapper, address, &mapped)) return -1;
    return (int)(mapped / 0x4000);
}

static void window_bytes(nes_t *nes, u16 address, char *out, size_t size) {
    snprintf(out, size, "%02X %02X %02X %02X", peek(nes, address), peek(nes, (u16)(address + 1)),
             peek(nes, (u16)(address + 2)), peek(nes, (u16)(address + 3)));
}

static int operand_bytes(const instruction_t *ins) {
    if (ins->addrmode == cpu_IMP) return 0;
    if (ins->addrmode == cpu_ABS || ins->addrmode == cpu_ABX ||
        ins->addrmode == cpu_ABY || ins->addrmode == cpu_IND) return 2;
    return 1;
}

typedef struct {
    u16 pc;
    u8 bytes[3];
    int length;
    const instruction_t *ins;
    u16 target;
    u8 a, x, y;
} step_t;

static step_t capture(nes_t *nes) {
    step_t s = { 0 };
    s.pc = nes->cpu.pc;
    s.bytes[0] = peek(nes, s.pc);
    s.ins = &cpu_lookup[s.bytes[0]];
    s.length = 1 + operand_bytes(s.ins);
    for (int i = 1; i < s.length; i++) s.bytes[i] = peek(nes, (u16)(s.pc + i));
    const u16 abs = (u16)(s.bytes[2] << 8) | s.bytes[1];
    s.target = abs;
    if (s.ins->addrmode == cpu_ABX) s.target = (u16)(abs + nes->cpu.x);
    if (s.ins->addrmode == cpu_ABY) s.target = (u16)(abs + nes->cpu.y);
    if (s.ins->addrmode == cpu_IZY) {
        const u16 base = (u16)(peek(nes, (u8)(s.bytes[1] + 1)) << 8) | peek(nes, s.bytes[1]);
        s.target = (u16)(base + nes->cpu.y);
    }
    s.a = nes->cpu.a;
    s.x = nes->cpu.x;
    s.y = nes->cpu.y;
    return s;
}

static int written_value(const step_t *s) {
    if (s->ins->operate == cpu_STA) return s->a;
    if (s->ins->operate == cpu_STX) return s->x;
    if (s->ins->operate == cpu_STY) return s->y;
    return -1;
}

static void sleep_ms(long ms) {
    const struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    const char *rom = NULL;
    long frames = 10;
    long limit = 40;
    long delay_ms = 0;
    u16 shown_at = 0x8000;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            frames = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc)
            limit = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--delay") == 0 && i + 1 < argc)
            delay_ms = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--peek") == 0 && i + 1 < argc)
            shown_at = (u16)strtol(argv[++i], NULL, 16);
        else
            rom = argv[i];
    }
    if (!rom) {
        fprintf(stderr, "usage: bank_trace rom.nes [--frames N] [--limit N] [--delay MS] [--peek HEX]\n");
        return 2;
    }

    cartridge_t *cart = cartridge_load(rom);
    if (!cart) {
        fprintf(stderr, "Could not load %s (missing file or unsupported mapper)\n", rom);
        return 1;
    }

    static nes_t nes;
    nes_insert_cartridge(&nes, cart);
    nes_reset(&nes);

    printf("[map] mapper=%u prg_banks=%u (%zu B)  $8000-$BFFF -> bank %d  $C000-$FFFF -> bank %d\n",
           cart->mapper_id, cart->prg_banks, cart->prg_size, window_bank(cart, 0x8000), window_bank(cart, 0xC000));

    long printed = 0;
    step_t step = capture(&nes);
    for (long frame = 0; frame < frames && printed < limit; frame++) {
        do {
            const bool boundary = nes.system_clock % 3 == 0 && cpu_complete(&nes) && !nes.ppu.nmi;
            if (boundary) step = capture(&nes);
            const int before = window_bank(cart, 0x8000);
            char bytes_before[16];
            window_bytes(&nes, shown_at, bytes_before, sizeof bytes_before);

            nes_clock(&nes);

            const int after = window_bank(cart, 0x8000);
            if (after != before && printed < limit) {
                char bytes_after[16];
                window_bytes(&nes, shown_at, bytes_after, sizeof bytes_after);
                char code[12] = "";
                for (int i = 0; i < step.length; i++)
                    snprintf(code + strlen(code), sizeof code - strlen(code), i ? " %02X" : "%02X", step.bytes[i]);
                const int value = written_value(&step);
                char shown[8] = "?";
                if (value >= 0) snprintf(shown, sizeof shown, "$%02X", value);
                printf("frame %3ld  %04X  %-8s  %s $%04X  value=%-3s  bank %d -> %d   $%04X: %s -> %s\n",
                       frame, step.pc, code, step.ins->name, step.target,
                       shown, before, after, shown_at, bytes_before, bytes_after);
                fflush(stdout);
                printed++;
                if (delay_ms > 0) sleep_ms(delay_ms);
            }
        } while (!nes.ppu.frame_complete);
        nes.ppu.frame_complete = false;
    }

    cartridge_free(cart);
    return 0;
}
