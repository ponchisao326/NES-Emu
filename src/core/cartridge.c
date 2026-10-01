//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include "core/cartridge.h"

cartridge_t *cartridge_load(const char *path) {
    (void)path;
    return NULL;
}

void cartridge_free(cartridge_t *cart) {
    (void)cart;
}

bool cart_cpu_read(cartridge_t *cart, u16 address, u8 *data) {
    (void)cart; (void)address; (void)data;
    return false;
}

bool cart_cpu_write(cartridge_t *cart, u16 address, u8 data) {
    (void)cart; (void)address; (void)data;
    return false;
}

bool cart_ppu_read(cartridge_t *cart, u16 address, u8 *data) {
    (void)cart; (void)address; (void)data;
    return false;
}

bool cart_ppu_write(cartridge_t *cart, u16 address, u8 data) {
    (void)cart; (void)address; (void)data;
    return false;
}
