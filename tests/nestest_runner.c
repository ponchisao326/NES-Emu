//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include <stdio.h>

#include "core/nes.h"
#include "core/bus.h"
#include "core/cartridge.h"
#include "core/cpu6502.h"
#include "core/cpu6502_tables.h"

#define NESTEST_START 0xC000
#define NESTEST_END 0xC66E
#define NESTEST_INITIAL_CYCLE 7
#define NESTEST_UNOFFICIAL_START 0xC6BD
#define NESTEST_UNOFFICIAL_START_CYCLE 14579
#define MAX_INSTRUCTIONS 10000
#define SKIP_RETURN_CODE 77

int main(int argc, char **argv) {
    const char *rom = (argc > 1) ? argv[1] : "roms/nestest.nes";

    cartridge_t *cart = cartridge_load(rom);
    if (!cart) {
        fprintf(stderr, "Skipped: could not load %s\n", rom);
        return SKIP_RETURN_CODE;
    }

    static nes_t nes;
    nes_insert_cartridge(&nes, cart);
    nes_reset(&nes);
    nes.cpu.pc = NESTEST_START;
    nes.cpu.cycles = 0;

    u64 cycles = NESTEST_INITIAL_CYCLE;
    u64 unofficial_start_cycle = 0;
    int executed = 0;

    while (executed < MAX_INSTRUCTIONS && nes.cpu.pc != NESTEST_END) {
        if (nes.cpu.pc == NESTEST_UNOFFICIAL_START && unofficial_start_cycle == 0)
            unofficial_start_cycle = cycles;

        if (cpu_lookup[bus_cpu_read(&nes, nes.cpu.pc, true)].operate == cpu_XXX)
            break;

        do { cpu_clock(&nes); cycles++; } while (!cpu_complete(&nes));
        executed++;
    }

    const u8 official = nes.cpu_ram[0x02];
    const u8 unofficial = nes.cpu_ram[0x03];
    const bool finished = nes.cpu.pc == NESTEST_END;

    printf("%d instructions, stopped at PC=%04X%s\n",
           executed, nes.cpu.pc, finished ? "" : " (unimplemented opcode)");
    printf("official:   %02X, cycle at $%04X = %llu (expected %d)\n",
           official, NESTEST_UNOFFICIAL_START,
           (unsigned long long)unofficial_start_cycle, NESTEST_UNOFFICIAL_START_CYCLE);
    printf("unofficial: %02X%s\n", unofficial, finished ? "" : " (incomplete)");

    cartridge_free(cart);

    if (official != 0x00 || unofficial_start_cycle != NESTEST_UNOFFICIAL_START_CYCLE) {
        fprintf(stderr, "Error: official opcodes failed\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}
