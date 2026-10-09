//
// Created by Victor Gomez Ponce on 09/10/2026.
//
// Runs nestest.nes from $C000 and prints one line per instruction in the
// same format as nestest.log, so both files can be compared with diff.
//
//   nestest_trace [rom] [--delay MS] [--color]
//
//   --delay MS  waits MS milliseconds after each line (for screen recordings)
//   --color     ANSI colours (only for the terminal, never for diff)
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

#define NESTEST_START 0xC000
#define NESTEST_END 0xC66E
#define NESTEST_INITIAL_CYCLE 7
#define MAX_INSTRUCTIONS 10000
#define PPU_DOTS_PER_LINE 341
#define PPU_LINES_PER_FRAME 262

static u8 peek(nes_t *nes, u16 address) {
    return bus_cpu_read(nes, address, true);
}

static u16 peek16_zp(nes_t *nes, u8 address) {
    return (u16)(peek(nes, (u8)(address + 1)) << 8) | peek(nes, address); // wraps inside page zero
}

// Operand text as nestest.log writes it, with the memory value before the instruction runs
static void disassemble(nes_t *nes, u16 pc, char *out, size_t size) {
    const u8 opcode = peek(nes, pc);
    const instruction_t *ins = &cpu_lookup[opcode];
    const u8 lo = peek(nes, pc + 1);
    const u8 hi = peek(nes, pc + 2);
    const u16 abs = (u16)(hi << 8) | lo;
    const char *name = (ins->name[0] == '?') ? (ins->operate == cpu_SBC ? "SBC" : "NOP") : ins->name;
    const bool jump = ins->operate == cpu_JMP || ins->operate == cpu_JSR;

    if (ins->addrmode == cpu_IMP) {
        const bool accumulator = opcode == 0x0A || opcode == 0x2A || opcode == 0x4A || opcode == 0x6A;
        snprintf(out, size, accumulator ? "%s A" : "%s", name);
    } else if (ins->addrmode == cpu_IMM) {
        snprintf(out, size, "%s #$%02X", name, lo);
    } else if (ins->addrmode == cpu_ZP0) {
        snprintf(out, size, "%s $%02X = %02X", name, lo, peek(nes, lo));
    } else if (ins->addrmode == cpu_ZPX) {
        const u8 a = (u8)(lo + nes->cpu.x);
        snprintf(out, size, "%s $%02X,X @ %02X = %02X", name, lo, a, peek(nes, a));
    } else if (ins->addrmode == cpu_ZPY) {
        const u8 a = (u8)(lo + nes->cpu.y);
        snprintf(out, size, "%s $%02X,Y @ %02X = %02X", name, lo, a, peek(nes, a));
    } else if (ins->addrmode == cpu_REL) {
        snprintf(out, size, "%s $%04X", name, (u16)(pc + 2 + (s8)lo));
    } else if (ins->addrmode == cpu_ABS) {
        if (jump)
            snprintf(out, size, "%s $%04X", name, abs);
        else
            snprintf(out, size, "%s $%04X = %02X", name, abs, peek(nes, abs));
    } else if (ins->addrmode == cpu_ABX) {
        const u16 a = abs + nes->cpu.x;
        snprintf(out, size, "%s $%04X,X @ %04X = %02X", name, abs, a, peek(nes, a));
    } else if (ins->addrmode == cpu_ABY) {
        const u16 a = abs + nes->cpu.y;
        snprintf(out, size, "%s $%04X,Y @ %04X = %02X", name, abs, a, peek(nes, a));
    } else if (ins->addrmode == cpu_IND) {
        const u16 hi_addr = (abs & 0xFF00) | (u8)(lo + 1); // the 6502 page-wrap bug
        const u16 target = (u16)(peek(nes, hi_addr) << 8) | peek(nes, abs);
        snprintf(out, size, "%s ($%04X) = %04X", name, abs, target);
    } else if (ins->addrmode == cpu_IZX) {
        const u8 zp = (u8)(lo + nes->cpu.x);
        const u16 a = peek16_zp(nes, zp);
        snprintf(out, size, "%s ($%02X,X) @ %02X = %04X = %02X", name, lo, zp, a, peek(nes, a));
    } else if (ins->addrmode == cpu_IZY) {
        const u16 base = peek16_zp(nes, lo);
        const u16 a = base + nes->cpu.y;
        snprintf(out, size, "%s ($%02X),Y = %04X @ %04X = %02X", name, lo, base, a, peek(nes, a));
    } else {
        snprintf(out, size, "%s", name);
    }
}

// The table names the one-byte NOPs at $1A $3A $5A $7A $DA $FA "NOP", but they are unofficial
static bool unofficial(u8 opcode) {
    if (cpu_lookup[opcode].name[0] == '?') return true;
    return opcode == 0x1A || opcode == 0x3A || opcode == 0x5A ||
           opcode == 0x7A || opcode == 0xDA || opcode == 0xFA;
}

static int operand_bytes(const instruction_t *ins) {
    if (ins->addrmode == cpu_IMP) return 0;
    if (ins->addrmode == cpu_ABS || ins->addrmode == cpu_ABX ||
        ins->addrmode == cpu_ABY || ins->addrmode == cpu_IND) return 2;
    return 1;
}

static void sleep_ms(long ms) {
    const struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    const char *rom = "roms/nestest.nes";
    long delay_ms = 0;
    bool color = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--delay") == 0 && i + 1 < argc)
            delay_ms = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--color") == 0)
            color = true;
        else
            rom = argv[i];
    }

    cartridge_t *cart = cartridge_load(rom);
    if (!cart) {
        fprintf(stderr, "Could not load %s\n", rom);
        return 1;
    }

    static nes_t nes;
    nes_insert_cartridge(&nes, cart);
    nes_reset(&nes);
    nes.cpu.pc = NESTEST_START;
    nes.cpu.cycles = 0;

    // nestest.log starts with the PPU 21 dots ahead (7 CPU cycles x 3)
    u64 cycles = NESTEST_INITIAL_CYCLE;
    int executed = 0;

    const char *dim = color ? "\x1b[2m" : "";
    const char *pc_col = color ? "\x1b[36m" : "";
    const char *op_col = color ? "\x1b[1;33m" : "";
    const char *reset = color ? "\x1b[0m" : "";

    while (executed < MAX_INSTRUCTIONS && nes.cpu.pc != NESTEST_END) {
        const u16 pc = nes.cpu.pc;
        const u8 opcode = peek(&nes, pc);
        const instruction_t *ins = &cpu_lookup[opcode];

        if (ins->operate == cpu_XXX)
            break;

        char bytes[16];
        char text[64];
        const int n = operand_bytes(ins);
        if (n == 0) snprintf(bytes, sizeof bytes, "%02X", opcode);
        if (n == 1) snprintf(bytes, sizeof bytes, "%02X %02X", opcode, peek(&nes, pc + 1));
        if (n == 2) snprintf(bytes, sizeof bytes, "%02X %02X %02X", opcode, peek(&nes, pc + 1), peek(&nes, pc + 2));
        disassemble(&nes, pc, text, sizeof text);

        const u64 dots = cycles * 3;
        const int scanline = (int)((dots / PPU_DOTS_PER_LINE) % PPU_LINES_PER_FRAME);
        const int dot = (int)(dots % PPU_DOTS_PER_LINE);

        printf("%s%04X%s  %s%-8s%s %c%s%-32s%sA:%02X X:%02X Y:%02X P:%02X SP:%02X %sPPU:%3d,%3d CYC:%llu%s\n",
               pc_col, pc, reset,
               dim, bytes, reset,
               unofficial(opcode) ? '*' : ' ',
               op_col, text, reset,
               nes.cpu.a, nes.cpu.x, nes.cpu.y, nes.cpu.status.reg, nes.cpu.stkp,
               dim, scanline, dot, (unsigned long long)cycles, reset);

        if (delay_ms > 0) {
            fflush(stdout);
            sleep_ms(delay_ms);
        }

        do { cpu_clock(&nes); cycles++; } while (!cpu_complete(&nes));
        executed++;
    }

    fprintf(stderr, "%d instructions, stopped at PC=%04X\n", executed, nes.cpu.pc);
    cartridge_free(cart);
    return 0;
}
