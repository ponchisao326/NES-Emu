//
// Created by Victor Gomez Ponce on 30/09/2026.
//
#include <stdio.h>

#include "common/types.h"
#include "core/nes.h"
#include "core/bus.h"
#include "core/cpu6502.h"

static void dump_state(nes_t *nes) {
    printf("\nA:%02X  X:%02X  Y:%02X  SP:%02X  PC:%04X  CYC:%llu\n",
           nes->cpu.a, nes->cpu.x, nes->cpu.y, nes->cpu.stkp,
           nes->cpu.pc, (unsigned long long)nes->system_clock);

    printf("STATUS: %c%c%c%c%c%c%c%c\n",
           nes->cpu.status.n ? 'N' : '.', nes->cpu.status.v ? 'V' : '.',
           nes->cpu.status.u ? 'U' : '.', nes->cpu.status.b ? 'B' : '.',
           nes->cpu.status.d ? 'D' : '.', nes->cpu.status.i ? 'I' : '.',
           nes->cpu.status.z ? 'Z' : '.', nes->cpu.status.c ? 'C' : '.');
}

static void dump_ram(nes_t *nes, u16 addr, int rows) {
    for (int r = 0; r < rows; r++) {
        printf("$%04X:", addr);
        for (int c = 0; c < 16; c++)
            printf(" %02X", bus_cpu_read(nes, addr++, true));
        printf("\n");
    }
}

int main(void) {
    static nes_t nes;

    // 3 * 10
    const u8 prog[] = {
        0xA2,0x0A, 0x8E,0x00,0x00, 0xA2,0x03, 0x8E,0x01,0x00,
        0xAC,0x00,0x00, 0xA9,0x00, 0x18, 0x6D,0x01,0x00,
        0x88, 0xD0,0xFA, 0x8D,0x02,0x00, 0xEA, 0xEA, 0xEA
    };
    for (size_t i = 0; i < sizeof(prog); i++)
        bus_cpu_write(&nes, (u16)(0x8000 + i), prog[i]);

    bus_cpu_write(&nes, 0xFFFC, 0x00);
    bus_cpu_write(&nes, 0xFFFD, 0x80);
    nes_reset(&nes);

    do { cpu_clock(&nes); } while (nes.cpu.cycles > 0);

    for (int step = 0; step < 30; step++) {
        do { cpu_clock(&nes); } while (nes.cpu.cycles > 0);
        dump_state(&nes);
        dump_ram(&nes, 0x0000, 2);
        getchar();
    }
    return 0;
}