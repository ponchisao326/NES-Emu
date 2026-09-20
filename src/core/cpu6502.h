//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#pragma once
#include "common/types.h"

typedef struct nes nes_t;

typedef union {
    struct {
        u8 c : 1; // bit 0 - carry
        u8 z : 1; // bit 1 - zero
        u8 i : 1; // bit 2 - disable irq
        u8 d : 1; // bit 3 - decimal (Ignored by NES)
        u8 b : 1; // bit 4 - break
        u8 u : 1; // bit 5 - unused, siempre 1
        u8 v : 1; // bit 6 - overflow
        u8 n : 1; // bit 7 - negative
    };
    u8 reg;
} cpu_status_t;

_Static_assert(sizeof(cpu_status_t) == 1, "Status must be 1 byte long");

typedef struct {
    // Registries
    u8  a, x, y;
    u8  stkp;
    u16 pc;
    cpu_status_t status;

    // Emulation Scratch
    u8  fetched;
    u16 addr_abs;
    u16 addr_rel;
    u8  opcode;
    u8  cycles;
} cpu6502_t;

void cpu_reset(nes_t *nes);
void cpu_clock(nes_t *nes);
void cpu_irq  (nes_t *nes);
void cpu_nmi  (nes_t *nes);

u8 cpu_IMP(nes_t *nes); u8 cpu_IMM(nes_t *nes);
u8 cpu_ZP0(nes_t *nes); u8 cpu_ZPX(nes_t *nes); u8 cpu_ZPY(nes_t *nes);
u8 cpu_REL(nes_t *nes); u8 cpu_ABS(nes_t *nes);
u8 cpu_ABX(nes_t *nes); u8 cpu_ABY(nes_t *nes);
u8 cpu_IND(nes_t *nes); u8 cpu_IZX(nes_t *nes); u8 cpu_IZY(nes_t *nes);

// opcodes
u8 cpu_ADC(nes_t *nes); u8 cpu_AND(nes_t *nes); u8 cpu_ASL(nes_t *nes);
u8 cpu_BCC(nes_t *nes); u8 cpu_BCS(nes_t *nes); u8 cpu_BEQ(nes_t *nes);
u8 cpu_BIT(nes_t *nes); u8 cpu_BMI(nes_t *nes); u8 cpu_BNE(nes_t *nes);
u8 cpu_BPL(nes_t *nes); u8 cpu_BRK(nes_t *nes); u8 cpu_BVC(nes_t *nes);
u8 cpu_BVS(nes_t *nes); u8 cpu_CLC(nes_t *nes); u8 cpu_CLD(nes_t *nes);
u8 cpu_CLI(nes_t *nes); u8 cpu_CLV(nes_t *nes); u8 cpu_CMP(nes_t *nes);
u8 cpu_CPX(nes_t *nes); u8 cpu_CPY(nes_t *nes); u8 cpu_DEC(nes_t *nes);
u8 cpu_DEX(nes_t *nes); u8 cpu_DEY(nes_t *nes); u8 cpu_EOR(nes_t *nes);
u8 cpu_INC(nes_t *nes); u8 cpu_INX(nes_t *nes); u8 cpu_INY(nes_t *nes);
u8 cpu_JMP(nes_t *nes); u8 cpu_JSR(nes_t *nes); u8 cpu_LDA(nes_t *nes);
u8 cpu_LDX(nes_t *nes); u8 cpu_LDY(nes_t *nes); u8 cpu_LSR(nes_t *nes);
u8 cpu_NOP(nes_t *nes); u8 cpu_ORA(nes_t *nes); u8 cpu_PHA(nes_t *nes);
u8 cpu_PHP(nes_t *nes); u8 cpu_PLA(nes_t *nes); u8 cpu_PLP(nes_t *nes);
u8 cpu_ROL(nes_t *nes); u8 cpu_ROR(nes_t *nes); u8 cpu_RTI(nes_t *nes);
u8 cpu_RTS(nes_t *nes); u8 cpu_SBC(nes_t *nes); u8 cpu_SEC(nes_t *nes);
u8 cpu_SED(nes_t *nes); u8 cpu_SEI(nes_t *nes); u8 cpu_STA(nes_t *nes);
u8 cpu_STX(nes_t *nes); u8 cpu_STY(nes_t *nes); u8 cpu_TAX(nes_t *nes);
u8 cpu_TAY(nes_t *nes); u8 cpu_TSX(nes_t *nes); u8 cpu_TXA(nes_t *nes);
u8 cpu_TXS(nes_t *nes); u8 cpu_TYA(nes_t *nes);

// Ilegal opcodes
u8 cpu_XXX(nes_t *nes);