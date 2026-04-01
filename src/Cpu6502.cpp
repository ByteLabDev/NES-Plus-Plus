// The NES CPU core is based on the 6502 processor and runs at approximately 1.79 MHz (1.66 MHz in a PAL NES).
// https://www.nesdev.org/wiki/CPU

#include "Cpu6502.h"
#include <stdio.h>
#include <iostream>
#include <vector>

Cpu6502::Cpu6502(Bus* busPtr) {
	bus = busPtr;

	instruction_lookup[0x00] = { "BRK", &Cpu6502::BRK, &Cpu6502::IMP, 7 };
	instruction_lookup[0xE2] = { "NOP", &Cpu6502::BRK, &Cpu6502::IMP, 2 };
}

void Cpu6502::init() {
}

void Cpu6502::step() {
	uint8_t opcode = bus->read(PC);
	PC++;
}

void Cpu6502::reset() {
	uint8_t lo = bus->read(0xFFFC);
	uint8_t hi = bus->read(0xFFFD);

	acc = 0;
	x_ind = 0;
	y_ind = 0;
	PC = (hi << 8) | lo;
	SP = 0xFD;
	stat = stat | 0b0000100;	// NVIB DIZC
}

uint8_t Cpu6502::NOP() {
	return 0;
}

uint8_t Cpu6502::BRK() {
	return 0;
}

uint8_t Cpu6502::IMP() {
	return 0;
}