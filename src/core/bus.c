//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/bus.h"
#include "core/nes.h"

u8 bus_cpu_read(nes_t *nes, u16 address, bool readonly) {
    (void)readonly; // debugger will use it
#ifdef BUS_FLAT_MEM
    return nes->flat_ram[address];
#else
    (void)nes; (void)address;
    return 0x00;
#endif
}

void bus_cpu_write(nes_t *nes, u16 address, u8 data) {
#ifdef BUS_FLAT_MEM
    nes->flat_ram[address] = data;
#else
    (void)nes; (void)address; (void)data;
#endif
}