//
// Created by Victor Gomez Ponce on 11/10/2026.
//
// Loads an iNES file with the emulator's own cartridge_load() and prints,
// in one line, what the loader found in the 16-byte header.
//
//   cart_info [rom]   (default: roms/nestest.nes)
//
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

#include "core/cartridge.h"

static char mirror_char(mirror_t m) {
    switch (m) {
        case MIRROR_HORIZONTAL: return 'H';
        case MIRROR_VERTICAL: return 'V';
        case MIRROR_ONESCREEN_LO: return 'L';
        case MIRROR_ONESCREEN_HI: return 'U';
    }
    return '?';
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "roms/nestest.nes";

    // the raw header, only for the two flag bytes the struct doesn't keep
    u8 header[16] = {0};
    FILE *f = fopen(path, "rb");
    if (!f || fread(header, 1, 16, f) != 16) {
        fprintf(stderr, "cart_info: can't read %s\n", path);
        if (f) fclose(f);
        return 1;
    }
    fclose(f);

    cartridge_t *cart = cartridge_load(path);
    if (!cart) {
        printf("[hdr] rejected by the loader: %s\n", path);
        return 1;
    }

    printf("[hdr] magic=NES$1A prg_banks=%u(%zu B) chr_banks=%u(%zu B) flags6=%02X flags7=%02X mapper=%u mirror=%c trainer=%d\n",
           cart->prg_banks, cart->prg_size, cart->chr_banks, cart->chr_size,
           header[6], header[7], cart->mapper_id, mirror_char(cart->mirror), (header[6] >> 2) & 1);

    cartridge_free(cart);
    return 0;
}
