#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/nes.h"
#include "core/bus.h"
#include "core/cartridge.h"
#include "core/cpu6502.h"
#include "core/cpu6502_tables.h"

static nes_t nes;

static const char *register_name(u16 address) {
    static const char *names[] = {
        "pulse1 duty/envelope", "pulse1 sweep", "pulse1 period lo", "pulse1 length/period hi",
        "pulse2 duty/envelope", "pulse2 sweep", "pulse2 period lo", "pulse2 length/period hi",
        "triangle linear", "-", "triangle period lo", "triangle length/period hi",
        "noise envelope", "-", "noise mode/period", "noise length",
        "dmc flags/rate", "dmc direct level", "dmc address", "dmc length"
    };
    if (address >= 0x4000 && address <= 0x4013) return names[address - 0x4000];
    if (address == 0x4015) return "status";
    if (address == 0x4017) return "frame counter";
    return NULL;
}

static int stored_register(u8 opcode) {
    const char *name = cpu_lookup[opcode].name;
    if (strcmp(name, "STA") == 0) return 'A';
    if (strcmp(name, "STX") == 0) return 'X';
    if (strcmp(name, "STY") == 0) return 'Y';
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <rom> <frames> [--from F] [--start F,F...]\n", argv[0]);
        return 2;
    }
    const int frames = atoi(argv[2]);
    int from = 0;
    int starts[64];
    int start_count = 0;
    for (int i = 3; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--from") == 0) from = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "--start") == 0) {
            for (char *tok = strtok(argv[i + 1], ","); tok && start_count < 64; tok = strtok(NULL, ","))
                starts[start_count++] = atoi(tok);
        }
    }

    cartridge_t *cart = cartridge_load(argv[1]);
    if (!cart) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    nes_insert_cartridge(&nes, cart);
    nes_reset(&nes);

    int frame = 0;
    u32 writes = 0;
    printf("# frame  line  dot  addr   value  register\n");
    while (frame < frames) {
        nes.controller[0] = 0x00;
        for (int i = 0; i < start_count; i++)
            if (frame >= starts[i] && frame < starts[i] + 4) nes.controller[0] = 0x10;

        const u8 opcode = bus_cpu_read(&nes, nes.cpu.pc, true);
        const int reg = stored_register(opcode);
        const u8 value = reg == 'A' ? nes.cpu.a : reg == 'X' ? nes.cpu.x : nes.cpu.y;
        const s16 line = nes.ppu.scanline;
        const s16 dot = nes.ppu.cycle;

        do { nes_clock(&nes); } while (!cpu_complete(&nes));
        do { nes_clock(&nes); } while (cpu_complete(&nes) && !nes.ppu.frame_complete);

        if (reg) {
            const char *name = register_name(nes.cpu.addr_abs);
            if (name && frame >= from) {
                printf("%7d  %4d  %3d  $%04X  $%02X    %s\n", frame, line, dot, nes.cpu.addr_abs, value, name);
                writes++;
            }
        }

        if (nes.ppu.frame_complete) {
            nes.ppu.frame_complete = false;
            frame++;
        }
    }
    printf("# %u writes in frames %d-%d\n", writes, from, frames - 1);
    cartridge_free(cart);
    return 0;
}
