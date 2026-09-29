//
// Created by Victor Gomez Ponce on 30/09/2026.
//
#include <stdio.h>
#include <string.h>

#include "common/types.h"
#include "core/nes.h"
#include "core/bus.h"
#include "core/cpu6502.h"

static nes_t nes;
static int failures;

static void poke(u16 addr, u8 v) { bus_cpu_write(&nes, addr, v); }

static void load(u16 addr, const u8 *code, size_t len) {
    for (size_t i = 0; i < len; i++)
        bus_cpu_write(&nes, (u16)(addr + i), code[i]);
}

// Wipes the machine and points the reset vector at $8000.
static void begin(const char *name) {
    memset(&nes, 0, sizeof nes);
    poke(0xFFFC, 0x00);
    poke(0xFFFD, 0x80);
    printf("\n== %s ==\n", name);
}

static void run(int instructions) {
    nes_reset(&nes);
    do { cpu_clock(&nes); } while (nes.cpu.cycles > 0); // burn the reset cycles
    for (int i = 0; i < instructions; i++)
        do { cpu_clock(&nes); } while (nes.cpu.cycles > 0);
}

static void check(const char *what, u16 addr, u8 expect) {
    u8 got = bus_cpu_read(&nes, addr, true);
    if (got == expect) {
        printf("  ok    %-24s $%04X = %02X\n", what, addr, got);
    } else {
        printf("  FAIL  %-24s $%04X = %02X  (expected %02X)\n", what, addr, got, expect);
        failures++;
    }
}

// tests

static void test_transfers(void) {
    static const u8 code[] = {
        0xA9,0x42, // LDA #$42
        0xAA, // TAX
        0x8E,0x00,0x00, // STX $0000
        0xA8, // TAY
        0x8C,0x01,0x00, // STY $0001
        0xA2,0x33, // LDX #$33
        0x8A, // TXA
        0x8D,0x02,0x00, // STA $0002
        0xA0,0x55, // LDY #$55
        0x98, // TYA
        0x8D,0x03,0x00, // STA $0003
        0xA2,0x7F, // LDX #$7F
        0x9A, // TXS
        0xA2,0x00, // LDX #$00
        0xBA, // TSX
        0x8E,0x04,0x00, // STX $0004
        0xEA,0xEA,0xEA,0xEA
    };
    begin("transfers  TAX TAY TXA TYA TXS TSX");
    load(0x8000, code, sizeof code);
    run(20);
    check("TAX", 0x0000, 0x42);
    check("TAY", 0x0001, 0x42);
    check("TXA", 0x0002, 0x33);
    check("TYA", 0x0003, 0x55);
    check("TXS+TSX", 0x0004, 0x7F);
}

static void test_incdec(void) {
    static const u8 code[] = {
        0xA9,0x05, // LDA #$05
        0x8D,0x00,0x00, // STA $0000
        0xEE,0x00,0x00, // INC $0000 -> 06
        0xCE,0x01,0x00, // DEC $0001 -> FF
        0xA2,0x10, // LDX #$10
        0xE8, // INX
        0x8E,0x02,0x00, // STX $0002 -> 11
        0xCA, // DEX
        0x8E,0x03,0x00, // STX $0003 -> 10
        0xA0,0x20, // LDY #$20
        0xC8, // INY
        0x8C,0x04,0x00, // STY $0004 -> 21
        0x88, // DEY
        0x8C,0x05,0x00, // STY $0005 -> 20
        0xEA,0xEA,0xEA,0xEA
    };
    begin("increment / decrement  INC DEC INX DEX INY DEY");
    load(0x8000, code, sizeof code);
    run(18);
    check("INC mem", 0x0000, 0x06);
    check("DEC mem wrap", 0x0001, 0xFF);
    check("INX", 0x0002, 0x11);
    check("DEX", 0x0003, 0x10);
    check("INY", 0x0004, 0x21);
    check("DEY", 0x0005, 0x20);
}

static void test_logic(void) {
    static const u8 code[] = {
        0xA9,0xF0, // LDA #$F0
        0x09,0x0F, // ORA #$0F -> FF
        0x8D,0x00,0x00, // STA $0000
        0xA9,0xFF, // LDA #$FF
        0x49,0x0F, // EOR #$0F -> F0
        0x8D,0x01,0x00, // STA $0001
        0xA9,0xCC, // LDA #$CC
        0x29,0x0F, // AND #$0F -> 0C
        0x8D,0x02,0x00, // STA $0002
        0xEA,0xEA,0xEA,0xEA
    };
    begin("logic  ORA EOR AND");
    load(0x8000, code, sizeof code);
    run(12);
    check("ORA", 0x0000, 0xFF);
    check("EOR", 0x0001, 0xF0);
    check("AND", 0x0002, 0x0C);
}

static void test_shifts(void) {
    static const u8 code[] = {
        0xA9,0x81, // LDA #$81
        0x0A, // ASL A -> 02, C=1
        0x8D,0x00,0x00, // STA $0000
        0xA9,0x81, // LDA #$81
        0x4A, // LSR A -> 40, C=1
        0x8D,0x01,0x00, // STA $0001
        0x18, // CLC
        0xA9,0x80, // LDA #$80
        0x2A, // ROL A -> 00, C=1
        0x2A, // ROL A -> 01, C=0
        0x8D,0x02,0x00, // STA $0002
        0x18, // CLC
        0xA9,0x01, // LDA #$01
        0x6A, // ROR A -> 00, C=1
        0x6A, // ROR A -> 80, C=0
        0x8D,0x03,0x00, // STA $0003
        0xA9,0x03, // LDA #$03
        0x8D,0x04,0x00, // STA $0004
        0x0E,0x04,0x00, // ASL $0004 -> 06
        0xA9,0x08, // LDA #$08
        0x8D,0x05,0x00, // STA $0005
        0x4E,0x05,0x00, // LSR $0005 -> 04
        0xEA,0xEA,0xEA,0xEA
    };
    begin("shifts  ASL LSR ROL ROR");
    load(0x8000, code, sizeof code);
    run(28);
    check("ASL acc", 0x0000, 0x02);
    check("LSR acc", 0x0001, 0x40);
    check("ROL carry-in", 0x0002, 0x01);
    check("ROR carry-in", 0x0003, 0x80);
    check("ASL mem", 0x0004, 0x06);
    check("LSR mem", 0x0005, 0x04);
}

static void test_compare(void) {
    static const u8 code[] = {
        0xA9,0x50, // LDA #$50
        0xC9,0x30, // CMP #$30 A > M -> C=1
        0xB0,0x02, // BCS +2
        0xA9,0x00, // LDA #$00 (skipped if right)
        0x8D,0x00,0x00, // STA $0000 -> 50

        0xA2,0x20, // LDX #$20
        0xE0,0x20, // CPX #$20 equal -> Z=1
        0xF0,0x02, // BEQ +2
        0xA2,0x00, // LDX #$00
        0x8E,0x01,0x00, // STX $0001 -> 20

        0xA0,0x10, // LDY #$10
        0xC0,0x20, // CPY #$20 Y < M -> C=0
        0x90,0x02, // BCC +2
        0xA0,0x00, // LDY #$00
        0x8C,0x02,0x00, // STY $0002 -> 10
        0xEA,0xEA,0xEA,0xEA
    };
    begin("compare  CMP CPX CPY");
    load(0x8000, code, sizeof code);
    run(16);
    check("CMP greater", 0x0000, 0x50);
    check("CPX equal", 0x0001, 0x20);
    check("CPY less", 0x0002, 0x10);
}

static void test_jsr_rts(void) {
    static const u8 code[] = {
        0x20,0x08,0x80, // $8000 JSR $8008
        0x8D,0x00,0x00, // $8003 STA $0000 -> 99
        0xEA, // $8006
        0xEA, // $8007
        0xA9,0x99, // $8008 LDA #$99
        0x60, // $800A RTS
        0xEA,0xEA,0xEA,0xEA
    };
    begin("subroutines  JSR RTS");
    load(0x8000, code, sizeof code);
    run(4);
    check("JSR + RTS", 0x0000, 0x99);
}

static void test_jmp(void) {
    static const u8 code[] = {
        0x4C,0x06,0x80, // $8000 JMP $8006
        0xA9,0x11, // $8003 LDA #$11 (must be skipped)
        0xEA, // $8005
        0xA9,0x22, // $8006 LDA #$22
        0x8D,0x00,0x00, // $8008 STA $0000 -> 22
        0x6C,0x20,0x00 // $800B JMP ($0020)
    };
    static const u8 far[] = {
        0xA9,0x33, // $8040 LDA #$33
        0x8D,0x01,0x00, // $8042 STA $0001 -> 33
        0xEA,0xEA
    };
    begin("jumps  JMP absolute + indirect");
    load(0x8000, code, sizeof code);
    load(0x8040, far, sizeof far);
    poke(0x0020, 0x40); // pointer -> $8040
    poke(0x0021, 0x80);
    run(6);
    check("JMP abs", 0x0000, 0x22);
    check("JMP ind", 0x0001, 0x33);
}

static void test_php_plp(void) {
    static const u8 code[] = {
        0x38, // SEC C=1
        0x08, // PHP
        0x18, // CLC C=0
        0x28, // PLP C back to 1
        0xA9,0x00, // LDA #$00
        0x90,0x02, // BCC +2 not taken if C survived
        0xA9,0xAA, // LDA #$AA
        0x8D,0x00,0x00, // STA $0000 -> AA
        0xEA,0xEA,0xEA,0xEA
    };
    begin("status on the stack  PHP PLP");
    load(0x8000, code, sizeof code);
    run(10);
    check("PHP + PLP", 0x0000, 0xAA);
}

static void test_bit(void) {
    static const u8 code[] = {
        0xA9,0xFF, // LDA #$FF
        0x24,0x10, // BIT $10 M=C0 -> N=1 V=1 Z=0
        0x70,0x02, // BVS +2
        0xA9,0xEE, // LDA #$EE
        0x8D,0x00,0x00, // STA $0000 -> FF

        0xA9,0xFF, // LDA #$FF
        0x24,0x10, // BIT $10
        0x30,0x02, // BMI +2
        0xA9,0xEE, // LDA #$EE
        0x8D,0x01,0x00, // STA $0001 -> FF

        0xA9,0x00, // LDA #$00
        0x24,0x10, // BIT $10 A&M = 0 -> Z=1
        0xF0,0x02, // BEQ +2
        0xA9,0xEE, // LDA #$EE
        0x8D,0x02,0x00, // STA $0002 -> 00
        0xEA,0xEA,0xEA,0xEA
    };
    begin("BIT");
    load(0x8000, code, sizeof code);
    poke(0x0010, 0xC0); // bit7 and bit6 set
    poke(0x0002, 0x99); // sentinel: 00 only if the store really happened
    run(16);
    check("BIT -> V", 0x0000, 0xFF);
    check("BIT -> N", 0x0001, 0xFF);
    check("BIT -> Z", 0x0002, 0x00);
}

static void test_brk_rti(void) {
    static const u8 code[] = {
        0x00, // $8000 BRK
        0xEA, // $8001 (padding byte BRK skips)
        0x8D,0x00,0x00, // $8002 STA $0000 -> 77 after RTI
        0xEA,0xEA,0xEA,0xEA
    };
    static const u8 handler[] = {
        0xA9,0x77, // $8050 LDA #$77
        0x40 // $8052 RTI
    };
    begin("interrupts  BRK RTI");
    load(0x8000, code, sizeof code);
    load(0x8050, handler, sizeof handler);
    poke(0xFFFE, 0x50); // IRQ/BRK vector -> $8050
    poke(0xFFFF, 0x80);
    run(4);
    check("BRK + RTI", 0x0000, 0x77);
}

// main

int main(void) {
    test_transfers();
    test_incdec();
    test_logic();
    test_shifts();
    test_compare();
    test_jsr_rts();
    test_jmp();
    test_php_plp();
    test_bit();
    test_brk_rti();

    printf("\n%s (%d failing)\n", failures ? "INCOMPLETE" : "ALL PASS", failures);
    return failures ? 1 : 0;
}
