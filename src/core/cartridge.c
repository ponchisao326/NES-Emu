//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include "core/cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mappers/mapper.h"

cartridge_t *cartridge_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    u8 header[16];
    if (fread(header, 1, 16, f) != 16) { fclose(f); return NULL; }

    if (memcmp(header, "NES\x1A", 4) != 0) { fclose(f); return NULL; }

    cartridge_t *cart = calloc(1, sizeof(cartridge_t));
    if (!cart) { fclose(f); return NULL; }

    cart->prg_banks = header[4];
    cart->chr_banks = header[5];
    cart->mapper_id = (header[7] & 0xF0) | (header[6] >> 4);
    cart->mirror = (header[6] & 0x01) ? MIRROR_VERTICAL : MIRROR_HORIZONTAL;

    if (header[6] & 0x04) fseek(f, 512, SEEK_CUR); // Trainer, gets discarded

    cart->prg_size = (size_t)cart->prg_banks * 16384;
    cart->prg_rom = malloc(cart->prg_size);
    fread(cart->prg_rom, 1, cart->prg_size, f);

    cart->chr_size = (size_t)cart->chr_banks * 8192;
    cart->chr_rom = malloc(cart->chr_size);
    fread(cart->chr_rom, 1, cart->chr_size, f);

    fclose(f);

    switch (cart->mapper_id) {
        case 0: mapper000_init(&cart->mapper, cart->prg_banks, cart->chr_banks); break;
        default:
            cartridge_free(cart);
            fclose(f);
            return NULL; // Not supported mapper
    }

    return cart;
}

void cartridge_free(cartridge_t *cart) {
    if (!cart) return;
    free(cart->prg_rom);
    free(cart->chr_rom);
    free(cart);
}

bool cart_cpu_read(cartridge_t *cart, u16 address, u8 *data) {
    if (!cart) return false;
    u32 mapped = 0;
    if (cart->mapper.cpu_map_read(&cart->mapper, address, &mapped)) {
        *data = cart->prg_rom[mapped];
        return true;
    }
    return false;
}

bool cart_cpu_write(cartridge_t *cart, u16 address, u8 data) {
    if (!cart) return false;
    u32 mapped = 0;
    if (cart->mapper.cpu_map_write(&cart->mapper, address, &mapped)) {
        cart->prg_rom[mapped] = data;
        return true;
    }
    return false;
}

bool cart_ppu_read(cartridge_t *cart, u16 address, u8 *data) {
    if (!cart) return false;
    u32 mapped = 0;
    if (cart->mapper.ppu_map_read(&cart->mapper, address, &mapped)) {
        *data = cart->chr_rom[mapped];
        return true;
    }
    return false;
}

bool cart_ppu_write(cartridge_t *cart, u16 address, u8 data) {
    if (!cart) return false;
    u32 mapped = 0;
    if (cart->mapper.ppu_map_write(&cart->mapper, address, &mapped)) {
        cart->chr_rom[mapped] = data;
        return true;
    }
    return false;
}