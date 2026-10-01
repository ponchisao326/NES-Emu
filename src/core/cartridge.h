//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"

typedef enum {
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    MIRROR_ONESCREEN_LO,
    MIRROR_ONESCREEN_HI
} mirror_t;

typedef struct cartridge {
    u8 *prg_rom;
    size_t prg_size;
    u8 *chr_rom;
    size_t chr_size;

    u8 prg_banks;
    u8 chr_banks;
    u8 mapper_id;
    mirror_t mirror;
} cartridge_t;

cartridge_t *cartridge_load(const char *path);
void cartridge_free(cartridge_t *cart);

// Main bus comms
bool cart_cpu_read(cartridge_t *cart, u16 address, u8 *data);
bool cart_cpu_write(cartridge_t *cart, u16 address, u8 data);

// PPU's bus comms
bool cart_ppu_read(cartridge_t *cart, u16 address, u8 *data);
bool cart_ppu_write(cartridge_t *cart, u16 address, u8 data);