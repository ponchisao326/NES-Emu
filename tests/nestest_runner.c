//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include <stdio.h>
#include "core/nes.h"
#include "core/bus.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    static nes_t nes;

    bus_cpu_write(&nes, 0xFFFC, 0x00);
    bus_cpu_write(&nes, 0xFFFD, 0x80);
    nes_reset(&nes);
    printf("PC after reset = %04X\n", nes.cpu.pc);

    bus_cpu_write(&nes, 0x00FF, 0x42);
    bus_cpu_write(&nes, 0x8000, 0xA9);

    u8 a = bus_cpu_read(&nes, 0x00FF, false);
    u8 b = bus_cpu_read(&nes, 0x8000, false);
    u8 c = bus_cpu_read(&nes, 0x1234, false);

    printf("%02X %02X %02X\n", a, b, c);

    if (a != 0x42 || b != 0xA9 || c != 0x00) {
        fprintf(stderr, "Error: expected 42 A9 00\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}