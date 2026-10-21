//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#pragma once
#include "common/types.h"
#include "core/mappers/mapper.h"

typedef struct cartridge {
    u8 *prg_rom;
    size_t prg_size;
    u8 *chr_rom;
    size_t chr_size;
    u8 prg_ram[0x2000];
    bool battery;

    u8 prg_banks;
    u8 chr_banks;
    u8 mapper_id;
    mirror_t mirror;

    mapper_t mapper;
} cartridge_t;

cartridge_t *cartridge_load(const char *path);
void cartridge_free(cartridge_t *cart);

// Main bus comms
bool cart_cpu_read(cartridge_t *cart, u16 address, u8 *data);
bool cart_cpu_write(cartridge_t *cart, u16 address, u8 data);

// PPU's bus comms
bool cart_ppu_read(cartridge_t *cart, u16 address, u8 *data);
bool cart_ppu_write(cartridge_t *cart, u16 address, u8 data);

mirror_t cart_mirror(const cartridge_t *cart);
bool cart_irq(const cartridge_t *cart);
void cart_scanline(cartridge_t *cart);

bool cart_load_ram(cartridge_t *cart, const char *path);
bool cart_save_ram(const cartridge_t *cart, const char *path);