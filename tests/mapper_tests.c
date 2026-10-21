#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/types.h"
#include "core/cartridge.h"
#include "core/mappers/mapper.h"

static int failures;

static void expect(const char *what, u32 got, u32 wanted) {
    if (got == wanted) {
        printf("  ok    %s\n", what);
        return;
    }
    printf("  FAIL  %-44s got %u, expected %u\n", what, got, wanted);
    failures++;
}

static cartridge_t *make_cart(u8 prg_16k, u8 chr_8k, u32 prg_unit, u32 chr_unit) {
    cartridge_t *cart = calloc(1, sizeof(cartridge_t));
    cart->prg_banks = prg_16k;
    cart->chr_banks = chr_8k;
    cart->prg_size = (size_t)prg_16k * 0x4000;
    cart->chr_size = (size_t)(chr_8k ? chr_8k : 1) * 0x2000;
    cart->prg_rom = malloc(cart->prg_size);
    cart->chr_rom = calloc(1, cart->chr_size);

    for (size_t i = 0; i < cart->prg_size; i++) cart->prg_rom[i] = (u8)(i / prg_unit);
    if (chr_8k)
        for (size_t i = 0; i < cart->chr_size; i++) cart->chr_rom[i] = (u8)(i / chr_unit);
    return cart;
}

static u8 cpu_peek(cartridge_t *cart, u16 address) {
    u8 data = 0xEE;
    cart_cpu_read(cart, address, &data);
    return data;
}

static u8 ppu_peek(cartridge_t *cart, u16 address) {
    u8 data = 0xEE;
    cart_ppu_read(cart, address, &data);
    return data;
}

static void mmc1_write(cartridge_t *cart, u16 address, u8 value) {
    for (int bit = 0; bit < 5; bit++)
        cart_cpu_write(cart, address, (u8)((value >> bit) & 0x01));
}

static void test_mmc1_power_on(void) {
    printf("\n== MMC1 power on ==\n");
    cartridge_t *cart = make_cart(8, 2, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    expect("$8000 starts on bank 0", cpu_peek(cart, 0x8000), 0);
    expect("$C000 is fixed to the last bank", cpu_peek(cart, 0xC000), 7);
    expect("PRG RAM answers at $6000", cart_cpu_write(cart, 0x6000, 0x5A), true);
    expect("PRG RAM keeps what was written", cpu_peek(cart, 0x6000), 0x5A);
    cartridge_free(cart);
}

static void test_mmc1_serial_port(void) {
    printf("\n== MMC1 serial port ==\n");
    cartridge_t *cart = make_cart(8, 2, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    cart_cpu_write(cart, 0xE000, 0x01);
    cart_cpu_write(cart, 0xE000, 0x01);
    expect("two bits in, bank not switched yet", cpu_peek(cart, 0x8000), 0);
    cart_cpu_write(cart, 0xE000, 0x00);
    cart_cpu_write(cart, 0xE000, 0x00);
    cart_cpu_write(cart, 0xE000, 0x00);
    expect("fifth bit commits bank 3", cpu_peek(cart, 0x8000), 3);

    cart_cpu_write(cart, 0xE000, 0x01);
    cart_cpu_write(cart, 0xE000, 0x80);
    mmc1_write(cart, 0xE000, 0x05);
    expect("bit 7 resets the shift register", cpu_peek(cart, 0x8000), 5);
    expect("$C000 stays on the last bank", cpu_peek(cart, 0xC000), 7);
    cartridge_free(cart);
}

static void test_mmc1_prg_modes(void) {
    printf("\n== MMC1 PRG modes ==\n");
    cartridge_t *cart = make_cart(8, 2, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    mmc1_write(cart, 0xE000, 0x05);
    mmc1_write(cart, 0x8000, 0x08);
    expect("mode 2: $8000 fixed to bank 0", cpu_peek(cart, 0x8000), 0);
    expect("mode 2: $C000 switches", cpu_peek(cart, 0xC000), 5);

    mmc1_write(cart, 0x8000, 0x00);
    expect("32 KB mode: low half ignores bit 0", cpu_peek(cart, 0x8000), 4);
    expect("32 KB mode: high half is the next bank", cpu_peek(cart, 0xC000), 5);
    cartridge_free(cart);
}

static void test_mmc1_chr_and_mirroring(void) {
    printf("\n== MMC1 CHR and mirroring ==\n");
    cartridge_t *cart = make_cart(8, 4, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    mmc1_write(cart, 0x8000, 0x1E);
    mmc1_write(cart, 0xA000, 0x03);
    mmc1_write(cart, 0xC000, 0x06);
    expect("4 KB mode: $0000 from CHR bank 0 register", ppu_peek(cart, 0x0000), 3);
    expect("4 KB mode: $1000 from CHR bank 1 register", ppu_peek(cart, 0x1000), 6);
    expect("control 2 means vertical", cart_mirror(cart), MIRROR_VERTICAL);

    mmc1_write(cart, 0x8000, 0x0F);
    expect("8 KB mode: low half ignores bit 0", ppu_peek(cart, 0x0000), 2);
    expect("8 KB mode: high half is the next 4 KB", ppu_peek(cart, 0x1000), 3);
    expect("control 3 means horizontal", cart_mirror(cart), MIRROR_HORIZONTAL);

    mmc1_write(cart, 0x8000, 0x0C);
    expect("control 0 means one screen, low", cart_mirror(cart), MIRROR_ONESCREEN_LO);
    cartridge_free(cart);
}

static void test_mmc1_ram_disable(void) {
    printf("\n== MMC1 PRG RAM enable ==\n");
    cartridge_t *cart = make_cart(8, 0, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    cart_cpu_write(cart, 0x6000, 0x42);
    mmc1_write(cart, 0xE000, 0x10);
    expect("bit 4 of the PRG register hides the RAM", cart_cpu_write(cart, 0x6000, 0x99), false);
    mmc1_write(cart, 0xE000, 0x00);
    expect("clearing it brings the old byte back", cpu_peek(cart, 0x6000), 0x42);
    expect("CHR RAM is writable", cart_ppu_write(cart, 0x1234, 0x77), true);
    expect("CHR RAM reads back", ppu_peek(cart, 0x1234), 0x77);
    cartridge_free(cart);
}

static void test_mmc1_surom(void) {
    printf("\n== MMC1 512 KB board ==\n");
    cartridge_t *cart = make_cart(32, 0, 0x4000, 0x1000);
    mapper001_init(&cart->mapper, cart->prg_banks, cart->chr_banks);

    expect("fixed bank in the first 256 KB", cpu_peek(cart, 0xC000), 15);
    mmc1_write(cart, 0xA000, 0x10);
    mmc1_write(cart, 0xE000, 0x02);
    expect("CHR bit 4 moves $8000 to the upper 256 KB", cpu_peek(cart, 0x8000), 18);
    expect("and the fixed bank with it", cpu_peek(cart, 0xC000), 31);
    cartridge_free(cart);
}

int main(void) {
    test_mmc1_power_on();
    test_mmc1_serial_port();
    test_mmc1_prg_modes();
    test_mmc1_chr_and_mirroring();
    test_mmc1_ram_disable();
    test_mmc1_surom();

    printf("\n%s (%d failing)\n", failures ? "INCOMPLETE" : "ALL PASS", failures);
    return failures ? 1 : 0;
}
