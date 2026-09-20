//
// Created by Victor Gomez Ponce on 20/09/2026.
//

#pragma once
#include "common/types.h"

typedef struct nes nes_t;

u8 bus_cpu_read(nes_t *nes, u16 address, bool readonly);
void bus_cpu_write(nes_t *nes, u16 address, u8 data);