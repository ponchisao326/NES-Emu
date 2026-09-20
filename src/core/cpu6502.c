//
// Created by Victor Gomez Ponce on 20/09/2026.
//
#include "core/cpu6502.h"
#include "core/bus.h"
#include "core/cpu6502_tables.h"
#include "core/nes.h"

u8 cpu_fetch(nes_t *nes) {
    if (cpu_lookup[nes->cpu.opcode].addrmode != cpu_IMP)
        nes->cpu.fetched = bus_cpu_read(nes, nes->cpu.addr_abs, false);
    return nes->cpu.fetched;
}

u8 cpu_IMP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_IMM(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ZP0(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ZPX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ZPY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_REL(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ABS(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ABX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ABY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_IND(nes_t *nes) { (void)nes; return 0; }
u8 cpu_IZX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_IZY(nes_t *nes) { (void)nes; return 0; }

// opcodes
u8 cpu_ADC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_AND(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ASL(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BCC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BCS(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BEQ(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BIT(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BMI(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BNE(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BPL(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BRK(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BVC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BVS(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CLC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CLD(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CLI(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CLV(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CMP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CPX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_CPY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_DEC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_DEX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_DEY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_EOR(nes_t *nes) { (void)nes; return 0; }
u8 cpu_INC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_INX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_INY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_JMP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_JSR(nes_t *nes) { (void)nes; return 0; }
u8 cpu_LDA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_LDX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_LDY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_LSR(nes_t *nes) { (void)nes; return 0; }
u8 cpu_NOP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ORA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_PHA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_PHP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_PLA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_PLP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ROL(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ROR(nes_t *nes) { (void)nes; return 0; }
u8 cpu_RTI(nes_t *nes) { (void)nes; return 0; }
u8 cpu_RTS(nes_t *nes) { (void)nes; return 0; }
u8 cpu_SBC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_SEC(nes_t *nes) { (void)nes; return 0; }
u8 cpu_SED(nes_t *nes) { (void)nes; return 0; }
u8 cpu_SEI(nes_t *nes) { (void)nes; return 0; }
u8 cpu_STA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_STX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_STY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TAX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TAY(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TSX(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TXA(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TXS(nes_t *nes) { (void)nes; return 0; }
u8 cpu_TYA(nes_t *nes) { (void)nes; return 0; }

// Ilegal opcodes
u8 cpu_XXX(nes_t *nes) { (void)nes; return 0; }

void cpu_reset(nes_t *nes) {
    nes->cpu.addr_abs = 0xFFFC;
    u16 lo = bus_cpu_read(nes, nes->cpu.addr_abs + 0, false);
    u16 hi = bus_cpu_read(nes, nes->cpu.addr_abs + 1, false);
    nes->cpu.pc = (hi << 8) | lo;

    nes->cpu.a = 0;
    nes->cpu.x = 0;
    nes->cpu.y = 0;
    nes->cpu.stkp = 0xFD;
    nes->cpu.status.reg = 0x00;
    nes->cpu.status.u = 1;

    nes->cpu.addr_rel = 0x0000;
    nes->cpu.addr_abs = 0x0000;
    nes->cpu.fetched  = 0x00;

    nes->cpu.cycles = 8;
}

void cpu_clock(nes_t *nes) {
    if (nes->cpu.cycles == 0) {
        nes->cpu.opcode = bus_cpu_read(nes, nes->cpu.pc, false);
        nes->cpu.pc++;

        nes->cpu.status.u = 1;

        nes->cpu.cycles = cpu_lookup[nes->cpu.opcode].cycles;

        u8 extra1 = cpu_lookup[nes->cpu.opcode].addrmode(nes);
        u8 extra2 = cpu_lookup[nes->cpu.opcode].operate(nes);

        nes->cpu.cycles += (extra1 & extra2);

        nes->cpu.status.u = 1;
    }

    nes->system_clock++;
    nes->cpu.cycles--;
}

void cpu_irq  (nes_t *nes) { (void)nes; }
void cpu_nmi  (nes_t *nes) { (void)nes; }