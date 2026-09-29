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

void cpu_irq(nes_t *nes) {
    if (nes->cpu.status.i == 0) {
        bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, (nes->cpu.pc >> 8) & 0x00FF);
        nes->cpu.stkp--;
        bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, nes->cpu.pc & 0x00FF);
        nes->cpu.stkp--;

        nes->cpu.status.b = 0;
        nes->cpu.status.u = 1;
        nes->cpu.status.i = 1;
        bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, nes->cpu.status.reg);
        nes->cpu.stkp--;

        nes->cpu.addr_abs = 0xFFFE;
        u16 lo = bus_cpu_read(nes, nes->cpu.addr_abs + 0, false);
        u16 hi = bus_cpu_read(nes, nes->cpu.addr_abs + 1, false);
        nes->cpu.pc = (hi << 8) | lo;

        nes->cpu.cycles = 7;
    }
}

void cpu_nmi(nes_t *nes) {
    bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, (nes->cpu.pc >> 8) & 0x00FF);
    nes->cpu.stkp--;
    bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, nes->cpu.pc & 0x00FF);
    nes->cpu.stkp--;

    nes->cpu.status.b = 0;
    nes->cpu.status.u = 1;
    nes->cpu.status.i = 1;
    bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, nes->cpu.status.reg);
    nes->cpu.stkp--;

    nes->cpu.addr_abs = 0xFFFA;
    u16 lo = bus_cpu_read(nes, nes->cpu.addr_abs + 0, false);
    u16 hi = bus_cpu_read(nes, nes->cpu.addr_abs + 1, false);
    nes->cpu.pc = (hi << 8) | lo;

    nes->cpu.cycles = 8;
}

// Addressing Modes

u8 cpu_IMP(nes_t *nes) {
    nes->cpu.fetched = nes->cpu.a;
    return 0;
}

u8 cpu_IMM(nes_t *nes) {
    nes->cpu.addr_abs = nes->cpu.pc++;
    return 0;
}

u8 cpu_ZP0(nes_t *nes) {
    nes->cpu.addr_abs = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;
    nes->cpu.addr_abs &= 0x00FF;
    return 0;
}

u8 cpu_ZPX(nes_t *nes) {
    nes->cpu.addr_abs = (bus_cpu_read(nes, nes->cpu.pc, false) + nes->cpu.x);
    nes->cpu.pc++;
    nes->cpu.addr_abs &= 0x00FF;
    return 0;
}

u8 cpu_ZPY(nes_t *nes) {
    nes->cpu.addr_abs = (bus_cpu_read(nes, nes->cpu.pc, false) + nes->cpu.y);
    nes->cpu.pc++;
    nes->cpu.addr_abs &= 0x00FF;
    return 0;
}
u8 cpu_REL(nes_t *nes) {
    nes->cpu.addr_rel = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    // Sign-extend to 16 bits if the offset is negative
    if (nes->cpu.addr_rel & 0x80)
        nes->cpu.addr_rel |= 0xFF00;

    return 0;
}

u8 cpu_ABS(nes_t *nes) {
    u16 lo = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;
    u16 hi = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    nes->cpu.addr_abs = (hi << 8) | lo;

    return 0;
}

u8 cpu_ABX(nes_t *nes) {
    u16 lo = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;
    u16 hi = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    nes->cpu.addr_abs = (hi << 8) | lo;
    nes->cpu.addr_abs += nes->cpu.x;

    if ((nes->cpu.addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

u8 cpu_ABY(nes_t *nes) {
    u16 lo = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;
    u16 hi = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    nes->cpu.addr_abs = (hi << 8) | lo;
    nes->cpu.addr_abs += nes->cpu.y;

    if ((nes->cpu.addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

u8 cpu_IND(nes_t *nes) {
    u16 ptr_lo = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;
    u16 ptr_hi = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    u16 ptr = (ptr_hi << 8) | ptr_lo;

    // Known 6502 bug: pointer at $xxFF reads the high byte from $xx00
    if (ptr_lo == 0x00FF) {
        nes->cpu.addr_abs = (bus_cpu_read(nes, ptr & 0xFF00, false) << 8) |  bus_cpu_read(nes, ptr + 0, false);
    } else {
        nes->cpu.addr_abs = (bus_cpu_read(nes, ptr + 1, false) << 8) |  bus_cpu_read(nes, ptr + 0, false);
    }

    return 0;
}

u8 cpu_IZX(nes_t *nes) {
    const u16 zp_ptr = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    u16 lo = bus_cpu_read(nes, (zp_ptr + (u16)nes->cpu.x) & 0x00FF, false);
    u16 hi = bus_cpu_read(nes, (zp_ptr + (u16)nes->cpu.x + 1) & 0x00FF, false);

    nes->cpu.addr_abs = (hi << 8) | lo;

    return 0;
}

u8 cpu_IZY(nes_t *nes) {
    const u16 zp_ptr = bus_cpu_read(nes, nes->cpu.pc, false);
    nes->cpu.pc++;

    u16 lo = bus_cpu_read(nes, (zp_ptr & 0x00FF), false);
    u16 hi = bus_cpu_read(nes, ((zp_ptr + 1) & 0x00FF), false);

    nes->cpu.addr_abs = (hi << 8) | lo;
    nes->cpu.addr_abs += nes->cpu.y;

    if ((nes->cpu.addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}

// opcodes
u8 cpu_AND(nes_t *nes) {
    cpu_fetch(nes);
    nes->cpu.a &= nes->cpu.fetched;
    nes->cpu.status.z = (nes->cpu.a == 0x00);
    nes->cpu.status.n = (nes->cpu.a & 0x80) != 0;
    return 1;
}

u8 cpu_ASL(nes_t *nes) { (void)nes; return 0; }

static u8 cpu_branch(nes_t *nes, bool take) {
    if (take) {
        nes->cpu.cycles++;
        nes->cpu.addr_abs = nes->cpu.pc + nes->cpu.addr_rel;

        if ((nes->cpu.addr_abs & 0xFF00) != (nes->cpu.pc & 0xFF00)) nes->cpu.cycles++;

        nes->cpu.pc = nes->cpu.addr_abs;
    }
    return 0;
}

u8 cpu_BCS(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.c == 1); }
u8 cpu_BCC(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.c == 0); }
u8 cpu_BEQ(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.z == 1); }
u8 cpu_BNE(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.z == 0); }
u8 cpu_BMI(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.n == 1); }
u8 cpu_BPL(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.n == 0); }
u8 cpu_BVS(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.v == 1); }
u8 cpu_BVC(nes_t *nes) { return cpu_branch(nes, nes->cpu.status.v == 0); }

u8 cpu_BIT(nes_t *nes) { (void)nes; return 0; }
u8 cpu_BRK(nes_t *nes) { (void)nes; return 0; }

u8 cpu_CLC(nes_t *nes) {
    nes->cpu.status.c = false;
    return 0;
}

u8 cpu_CLD(nes_t *nes) {
    nes->cpu.status.d = false;
    return 0;
}

u8 cpu_CLI(nes_t *nes) {
    nes->cpu.status.i = false;
    return 0;
}

u8 cpu_CLV(nes_t *nes) {
    nes->cpu.status.v = false;
    return 0;
}

u8 cpu_SEC(nes_t *nes) {
    nes->cpu.status.c = true;
    return 0;
}

u8 cpu_SED(nes_t *nes) {
    nes->cpu.status.d = true;
    return 0;
}

u8 cpu_SEI(nes_t *nes) {
    nes->cpu.status.i = true;
    return 0;
}

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

u8 cpu_PHA(nes_t *nes) {
    bus_cpu_write(nes, 0x0100 + nes->cpu.stkp, nes->cpu.a);
    nes->cpu.stkp--;
    return 0;
}

u8 cpu_PHP(nes_t *nes) { (void)nes; return 0; }

u8 cpu_PLA(nes_t *nes) {
    nes->cpu.stkp++;
    nes->cpu.a = bus_cpu_read(nes, 0x0100 + nes->cpu.stkp, false);
    nes->cpu.status.z = (nes->cpu.a == 0x00);
    nes->cpu.status.n = ((nes->cpu.a & 0x80) != 0);
    return 0;
}

u8 cpu_PLP(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ROL(nes_t *nes) { (void)nes; return 0; }
u8 cpu_ROR(nes_t *nes) { (void)nes; return 0; }
u8 cpu_RTI(nes_t *nes) { (void)nes; return 0; }
u8 cpu_RTS(nes_t *nes) { (void)nes; return 0; }

// Overflow
u8 cpu_ADC(nes_t *nes) {
    cpu_fetch(nes);
    const u16 temp = nes->cpu.a + nes->cpu.fetched + nes->cpu.status.c;

    nes->cpu.status.c = (temp > 255);
    nes->cpu.status.z = ((temp & 0x00FF) == 0);
    nes->cpu.status.n = ((temp & 0x0080) != 0);
    nes->cpu.status.v = ((~((u16)nes->cpu.a ^ (u16)nes->cpu.fetched) & ((u16)nes->cpu.a ^ temp)) & 0x0080) != 0;

    nes->cpu.a = temp & 0x00FF;
    return 1;
}

u8 cpu_SBC(nes_t *nes) {
    cpu_fetch(nes);

    // Invert Data
    const u16 value = nes->cpu.fetched ^ 0x00FF;

    const u16 temp = nes->cpu.a + value + nes->cpu.status.c;

    nes->cpu.status.c = (temp > 255);
    nes->cpu.status.z = ((temp & 0x00FF) == 0);
    nes->cpu.status.n = ((temp & 0x0080) != 0);
    nes->cpu.status.v = ((temp ^ (u16)nes->cpu.a) & (temp ^ value) & 0x0080) != 0;

    nes->cpu.a = temp & 0x00FF;
    return 1;
}

// End Overflow

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