//
// Created by Victor Gomez Ponce on 01/10/2026.
//
#include <stdlib.h>

#include "core/ppu.h"

#include "cartridge.h"
#include "core/nes.h"

const u8 ppu_colors[64][3] = {
    { 84,  84,  84}, {  0,  30, 116}, {  8,  16, 144}, { 48,   0, 136},
    { 68,   0, 100}, { 92,   0,  48}, { 84,   4,   0}, { 60,  24,   0},
    { 32,  42,   0}, {  8,  58,   0}, {  0,  64,   0}, {  0,  60,   0},
    {  0,  50,  60}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {152, 150, 152}, {  8,  76, 196}, { 48,  50, 236}, { 92,  30, 228},
    {136,  20, 176}, {160,  20, 100}, {152,  34,  32}, {120,  60,   0},
    { 84,  90,   0}, { 40, 114,   0}, {  8, 124,   0}, {  0, 118,  40},
    {  0, 102, 120}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, { 76, 154, 236}, {120, 124, 236}, {176,  98, 236},
    {228,  84, 236}, {236,  88, 180}, {236, 106, 100}, {212, 136,  32},
    {160, 170,   0}, {116, 196,   0}, { 76, 208,  32}, { 56, 204, 108},
    { 56, 180, 204}, { 60,  60,  60}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236},
    {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144},
    {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180},
    {160, 214, 228}, {160, 162, 160}, {  0,   0,   0}, {  0,   0,   0},
};

u8 ppu_cpu_read(nes_t *nes, u16 address, bool readonly) {
    u8 data = 0x00;

    switch (address) {
        case 0x0000: // Control
            break;
        case 0x0001: // Mask
            break;
        case 0x0002: // Status
            data = (nes->ppu.status.reg & 0xE0) | (nes->ppu.data_buffer & 0x1F);
            if (readonly) break;
            nes->ppu.status.vertical_blank = 0;
            nes->ppu.address_latch = 0;
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            break;
        case 0x0006: // PPU Address
            break;
        case 0x0007: { // PPU Data
            const bool palette = (nes->ppu.vram_addr.reg & 0x3FFF) >= 0x3F00;

            if (readonly) {
                data = palette ? ppu_read(nes, nes->ppu.vram_addr.reg, true) : nes->ppu.data_buffer;
                break;
            }

            data = nes->ppu.data_buffer;
            nes->ppu.data_buffer = ppu_read(nes, nes->ppu.vram_addr.reg, false);

            if (palette) data = nes->ppu.data_buffer;

            nes->ppu.vram_addr.reg += nes->ppu.ctrl.increment_mode ? 32 : 1;
            break;
        }
    }

    return data;
}

void ppu_cpu_write(nes_t *nes, u16 address, u8 data) {
    switch (address) {
        case 0x0000: // Control
            nes->ppu.ctrl.reg = data;
            nes->ppu.tram_addr.nametable_x = nes->ppu.ctrl.nametable_x;
            nes->ppu.tram_addr.nametable_y = nes->ppu.ctrl.nametable_y;
            break;
        case 0x0001: // Mask
            nes->ppu.mask.reg = data;
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            if (nes->ppu.address_latch == 0) {
                // First write
                nes->ppu.fine_x = data & 0x07;
                nes->ppu.tram_addr.coarse_x = data >> 3;
                nes->ppu.address_latch = 1;
            } else {
                // Second write
                nes->ppu.tram_addr.fine_y = data & 0x07;
                nes->ppu.tram_addr.coarse_y = data >> 3;
                nes->ppu.address_latch = 0;
            }
            break;
        case 0x0006: // PPU Address
            if (nes->ppu.address_latch == 0) {
                // First write
                nes->ppu.tram_addr.reg = (u16)((data & 0x3F) << 8) | (nes->ppu.tram_addr.reg & 0x00FF);
                nes->ppu.address_latch = 1;
            } else {
                // Second write
                nes->ppu.tram_addr.reg = (nes->ppu.tram_addr.reg & 0xFF00) | data;
                nes->ppu.vram_addr = nes->ppu.tram_addr;
                nes->ppu.address_latch = 0;
            }
            break;
        case 0x0007: // PPU Data
            ppu_write(nes, nes->ppu.vram_addr.reg, data);
            nes->ppu.vram_addr.reg += nes->ppu.ctrl.increment_mode ? 32 : 1;
            break;
    }
}

static u8 *nametable_cell(nes_t *nes, u16 address) {
    const mirror_t mirror = nes->cart ? nes->cart->mirror : MIRROR_HORIZONTAL;
    u8 table = 0;

    address &= 0x0FFF;

    switch (mirror) {
        case MIRROR_VERTICAL:     table = (address >> 10) & 0x01; break;
        case MIRROR_HORIZONTAL:   table = (address >> 11) & 0x01; break;
        case MIRROR_ONESCREEN_LO: table = 0; break;
        case MIRROR_ONESCREEN_HI: table = 1; break;
    }

    return &nes->ppu.name_table[table][address & 0x03FF];
}

static u8 *palette_cell(nes_t *nes, u16 address) {
    address &= 0x001F;
    if (address == 0x0010) address = 0x0000;
    if (address == 0x0014) address = 0x0004;
    if (address == 0x0018) address = 0x0008;
    if (address == 0x001C) address = 0x000C;
    return &nes->ppu.palette[address];
}

u8 ppu_read(nes_t *nes, u16 address, bool readonly) {
    (void)readonly;
    u8 data = 0x00;
    address &= 0x3FFF;

    if (nes->cart && cart_ppu_read(nes->cart, address, &data))
        return data;

    if (address >= 0x2000 && address <= 0x3EFF)
        data = *nametable_cell(nes, address);
    else if (address >= 0x3F00)
        data = *palette_cell(nes, address);

    return data;
}

void ppu_write(nes_t *nes, u16 address, u8 data) {
    address &= 0x3FFF;

    if (nes->cart && cart_ppu_write(nes->cart, address, data))
        return;

    if (address >= 0x2000 && address <= 0x3EFF)
        *nametable_cell(nes, address) = data;
    else if (address >= 0x3F00)
        *palette_cell(nes, address) = data;
}

// Move the pointer un tile right, crossing nametable if needed
static void increment_scroll_x(nes_t *nes) {
    if (!nes->ppu.mask.render_background && !nes->ppu.mask.render_sprites) return;

    if (nes->ppu.vram_addr.coarse_x == 31) {
        nes->ppu.vram_addr.coarse_x = 0;
        nes->ppu.vram_addr.nametable_x = !nes->ppu.vram_addr.nametable_x;
    } else {
        nes->ppu.vram_addr.coarse_x++;
    }
}

// Move pointer one scanline downwards
static void increment_scroll_y(nes_t *nes) {
    if (!nes->ppu.mask.render_background && !nes->ppu.mask.render_sprites) return;

    if (nes->ppu.vram_addr.fine_y < 7) {
        nes->ppu.vram_addr.fine_y++;
        return;
    }

    nes->ppu.vram_addr.fine_y = 0;

    if (nes->ppu.vram_addr.coarse_y == 29) {
        nes->ppu.vram_addr.coarse_y = 0;
        nes->ppu.vram_addr.nametable_y = !nes->ppu.vram_addr.nametable_y;
    } else if (nes->ppu.vram_addr.coarse_y == 31) {
        nes->ppu.vram_addr.coarse_y = 0;
    } else {
        nes->ppu.vram_addr.coarse_y++;
    }
}

static void transfer_address_x(nes_t *nes) {
    if (!nes->ppu.mask.render_background && !nes->ppu.mask.render_sprites) return;

    nes->ppu.vram_addr.nametable_x = nes->ppu.tram_addr.nametable_x;
    nes->ppu.vram_addr.coarse_x = nes->ppu.tram_addr.coarse_x;
}

static void transfer_address_y(nes_t *nes) {
    if (!nes->ppu.mask.render_background && !nes->ppu.mask.render_sprites) return;

    nes->ppu.vram_addr.fine_y = nes->ppu.tram_addr.fine_y;
    nes->ppu.vram_addr.nametable_y = nes->ppu.tram_addr.nametable_y;
    nes->ppu.vram_addr.coarse_y = nes->ppu.tram_addr.coarse_y;
}

static void load_background_shifters(nes_t *nes) {
    nes->ppu.bg_shifter_pattern_lo = (nes->ppu.bg_shifter_pattern_lo & 0xFF00) | nes->ppu.bg_next_tile_lsb;
    nes->ppu.bg_shifter_pattern_hi = (nes->ppu.bg_shifter_pattern_hi & 0xFF00) | nes->ppu.bg_next_tile_msb;

    nes->ppu.bg_shifter_attrib_lo = (nes->ppu.bg_shifter_attrib_lo & 0xFF00)
                                  | ((nes->ppu.bg_next_tile_attrib & 0x01) ? 0x00FF : 0x0000);
    nes->ppu.bg_shifter_attrib_hi = (nes->ppu.bg_shifter_attrib_hi & 0xFF00)
                                  | ((nes->ppu.bg_next_tile_attrib & 0x02) ? 0x00FF : 0x0000);
}

static void update_shifters(nes_t *nes) {
    if (!nes->ppu.mask.render_background) return;

    nes->ppu.bg_shifter_pattern_lo <<= 1;
    nes->ppu.bg_shifter_pattern_hi <<= 1;
    nes->ppu.bg_shifter_attrib_lo <<= 1;
    nes->ppu.bg_shifter_attrib_hi <<= 1;
}

void ppu_clock(nes_t *nes) {
    if (nes->ppu.scanline >= -1 && nes->ppu.scanline < 240) {
        if (nes->ppu.scanline == 0 && nes->ppu.cycle == 0) {
            nes->ppu.cycle = 1;
        }

        if (nes->ppu.scanline == -1 && nes->ppu.cycle == 1) {
            nes->ppu.status.vertical_blank = 0;
        }

        if ((nes->ppu.cycle >= 2 && nes->ppu.cycle < 258)
         || (nes->ppu.cycle >= 321 && nes->ppu.cycle < 338)) {
            update_shifters(nes);

            switch ((nes->ppu.cycle - 1) % 8) {
                case 0:
                    load_background_shifters(nes);
                    nes->ppu.bg_next_tile_id =
                        ppu_read(nes, 0x2000 | (nes->ppu.vram_addr.reg & 0x0FFF), false);
                    break;

                case 2:
                    nes->ppu.bg_next_tile_attrib =
                        ppu_read(nes, 0x23C0
                                    | (nes->ppu.vram_addr.nametable_y << 11)
                                    | (nes->ppu.vram_addr.nametable_x << 10)
                                    | ((nes->ppu.vram_addr.coarse_y >> 2) << 3)
                                    | (nes->ppu.vram_addr.coarse_x >> 2), false);

                    if (nes->ppu.vram_addr.coarse_y & 0x02) nes->ppu.bg_next_tile_attrib >>= 4;
                    if (nes->ppu.vram_addr.coarse_x & 0x02) nes->ppu.bg_next_tile_attrib >>= 2;
                    nes->ppu.bg_next_tile_attrib &= 0x03;
                    break;

                case 4:
                    nes->ppu.bg_next_tile_lsb =
                        ppu_read(nes, (nes->ppu.ctrl.pattern_background << 12)
                                    + ((u16)nes->ppu.bg_next_tile_id << 4)
                                    + nes->ppu.vram_addr.fine_y + 0, false);
                    break;

                case 6:
                    nes->ppu.bg_next_tile_msb =
                        ppu_read(nes, (nes->ppu.ctrl.pattern_background << 12)
                                    + ((u16)nes->ppu.bg_next_tile_id << 4)
                                    + nes->ppu.vram_addr.fine_y + 8, false);
                    break;

                case 7:
                    increment_scroll_x(nes);
                    break;
            }
        }

        if (nes->ppu.cycle == 256) increment_scroll_y(nes);

        if (nes->ppu.cycle == 257) {
            load_background_shifters(nes);
            transfer_address_x(nes);
        }

        if (nes->ppu.cycle == 338 || nes->ppu.cycle == 340) {
            nes->ppu.bg_next_tile_id =
                ppu_read(nes, 0x2000 | (nes->ppu.vram_addr.reg & 0x0FFF), false);
        }

        if (nes->ppu.scanline == -1 && nes->ppu.cycle >= 280 && nes->ppu.cycle < 305) {
            transfer_address_y(nes);
        }
    }

    if (nes->ppu.scanline == 241 && nes->ppu.cycle == 1) {
        nes->ppu.status.vertical_blank = 1;
        if (nes->ppu.ctrl.enable_nmi)
            nes->ppu.nmi = true;
    }

    u8 bg_pixel = 0x00;
    u8 bg_palette = 0x00;

    if (nes->ppu.mask.render_background
        && (nes->ppu.mask.render_background_left || nes->ppu.cycle > 8)) {
        const u16 bit_mux = 0x8000 >> nes->ppu.fine_x;

        const u8 p0 = (nes->ppu.bg_shifter_pattern_lo & bit_mux) != 0;
        const u8 p1 = (nes->ppu.bg_shifter_pattern_hi & bit_mux) != 0;
        bg_pixel = (u8)((p1 << 1) | p0);

        const u8 a0 = (nes->ppu.bg_shifter_attrib_lo & bit_mux) != 0;
        const u8 a1 = (nes->ppu.bg_shifter_attrib_hi & bit_mux) != 0;
        bg_palette = (u8)((a1 << 1) | a0);

        if (bg_pixel == 0x00) bg_palette = 0x00;
    }

    const s16 x = nes->ppu.cycle - 1;
    const s16 y = nes->ppu.scanline;
    if (x >= 0 && x < PPU_SCREEN_W && y >= 0 && y < PPU_SCREEN_H)
        nes->ppu.screen[y][x] = ppu_colour_from_palette(nes, bg_palette, bg_pixel);

    nes->ppu.cycle++;
    if (nes->ppu.cycle >= 341) {
        nes->ppu.cycle = 0;
        nes->ppu.scanline++;
        if (nes->ppu.scanline >= 261) {
            nes->ppu.scanline = -1;
            nes->ppu.frame_complete = true;
        }
    }
}

void ppu_reset(nes_t *nes) {
    nes->ppu.fine_x = 0x00;
    nes->ppu.address_latch = 0x00;
    nes->ppu.data_buffer = 0x00;
    nes->ppu.scanline = 0;
    nes->ppu.cycle = 0;
    nes->ppu.frame_complete = false;
    nes->ppu.nmi = false;

    nes->ppu.bg_next_tile_id = 0x00;
    nes->ppu.bg_next_tile_attrib = 0x00;
    nes->ppu.bg_next_tile_lsb = 0x00;
    nes->ppu.bg_next_tile_msb = 0x00;
    nes->ppu.bg_shifter_pattern_lo = 0x0000;
    nes->ppu.bg_shifter_pattern_hi = 0x0000;
    nes->ppu.bg_shifter_attrib_lo = 0x0000;
    nes->ppu.bg_shifter_attrib_hi = 0x0000;

    nes->ppu.status.reg = 0x00;
    nes->ppu.mask.reg = 0x00;
    nes->ppu.ctrl.reg = 0x00;
    nes->ppu.vram_addr.reg = 0x0000;
    nes->ppu.tram_addr.reg = 0x0000;
}

u8 ppu_colour_from_palette(nes_t *nes, u8 palette, u8 pixel) {
    return ppu_read(nes, 0x3F00 + (palette << 2) + pixel, false) & 0x3F;
}

void ppu_render_pattern_table(nes_t *nes, u8 index, u8 palette) {
    for (u16 tile_y = 0; tile_y < 16; tile_y++) {
        for (u16 tile_x = 0; tile_x < 16; tile_x++) {
            const u16 offset = tile_y * 256 + tile_x * 16;

            for (u16 row = 0; row < 8; row++) {
                u8 lsb = ppu_read(nes, index * 0x1000 + offset + row + 0x0000, false);
                u8 msb = ppu_read(nes, index * 0x1000 + offset + row + 0x0008, false);

                for (u16 col = 0; col < 8; col++) {
                    const u8 pixel = ((msb & 0x01) << 1) | (lsb & 0x01);
                    lsb >>= 1;
                    msb >>= 1;

                    nes->ppu.pattern_screen[index][tile_y * 8 + row][tile_x * 8 + (7 - col)] =
                        ppu_colour_from_palette(nes, palette, pixel);
                }
            }
        }
    }
}