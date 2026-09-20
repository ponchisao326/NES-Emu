//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include <stdio.h>
#include "core/nes.h"

int main(void) {
    static nes_t nes;
    nes_reset(&nes);
    printf("Nes_Emu: nes_t sizes %zu bytes\n", sizeof(nes_t));
    return 0;
}