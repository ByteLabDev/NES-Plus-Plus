// The NES CPU core is based on the 6502 processor and runs at approximately 1.79 MHz (1.66 MHz in a PAL NES).
// https://www.nesdev.org/wiki/CPU
// https://www.masswerk.at/6502/6502_instruction_set.html

#include "Cpu6502.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string> 
#include <algorithm>
#include <array>

Cpu6502::Cpu6502(Bus* busPtr) {
	bus = busPtr;

	// TEMP: Fill the instructions with blank data
	for (int i = 0; i < 256; i++) {
		instruction_lookup[i] = { "UNK", &Cpu6502::NOP, &Cpu6502::IMP, 2 };
	}

	instruction_lookup[0x00] = { "BRK", &Cpu6502::BRK, &Cpu6502::IMP, 7 };
	instruction_lookup[0xE2] = { "NOP", &Cpu6502::BRK, &Cpu6502::IMP, 2 };

	instruction_lookup[0xCC] = { "CPY", &Cpu6502::CPY, &Cpu6502::AB0, 4 };
	instruction_lookup[0x78] = { "SEI", &Cpu6502::SEI, &Cpu6502::IMP, 2 };
	instruction_lookup[0xD8] = { "CLD", &Cpu6502::CLD, &Cpu6502::IMP, 2 };
	instruction_lookup[0x8D] = { "STA", &Cpu6502::STA, &Cpu6502::AB0, 4 };

	instruction_lookup[0x9A] = { "TXS", &Cpu6502::STA, &Cpu6502::IMP, 2 };

	instruction_lookup[0x4C] = { "JMP", &Cpu6502::JMP, &Cpu6502::AB0, 3 };
	instruction_lookup[0x6C] = { "JMP", &Cpu6502::JMP, &Cpu6502::IND, 5 };

	instruction_lookup[0xA9] = { "LDA", &Cpu6502::LDA, &Cpu6502::IMM, 2 };
	instruction_lookup[0xA5] = { "LDA", &Cpu6502::LDA, &Cpu6502::ZP0, 3 };
	instruction_lookup[0xB5] = { "LDA", &Cpu6502::LDA, &Cpu6502::ZPX, 4 };
	instruction_lookup[0xAD] = { "LDA", &Cpu6502::LDA, &Cpu6502::AB0, 4 };
	instruction_lookup[0xBD] = { "LDA", &Cpu6502::LDA, &Cpu6502::ABX, 4 };
	instruction_lookup[0xB9] = { "LDA", &Cpu6502::LDA, &Cpu6502::ABY, 4 };
	instruction_lookup[0xA1] = { "LDA", &Cpu6502::LDA, &Cpu6502::IZX, 6 };
	instruction_lookup[0xB1] = { "LDA", &Cpu6502::LDA, &Cpu6502::IZY, 5 };

	instruction_lookup[0xA2] = { "LDX", &Cpu6502::LDX, &Cpu6502::IMM, 2 };
	instruction_lookup[0xA6] = { "LDX", &Cpu6502::LDX, &Cpu6502::ZP0, 3 };
	instruction_lookup[0xB6] = { "LDX", &Cpu6502::LDX, &Cpu6502::ZPY, 4 };
	instruction_lookup[0xAE] = { "LDX", &Cpu6502::LDX, &Cpu6502::AB0, 4 };
	instruction_lookup[0xBE] = { "LDX", &Cpu6502::LDX, &Cpu6502::ABY, 4 };
}

void Cpu6502::init() {
}

void Cpu6502::step() {
	uint8_t opcode = bus->read(PC++);
	std::cout << "PC: 0x" << std::hex << (int)PC << std::endl;
	std::cout << "Opcode: 0x" << std::hex << (int)opcode << std::endl;
	Instruction& inst = instruction_lookup[opcode];

	if (inst.name == "UNK" || inst.addrmode == nullptr || inst.operate == nullptr) {
		std::cout << "Unimplemented Opcode: 0x" << std::hex << (int)opcode << std::endl;
		PC--;
		return;
	}

	(this->*inst.addrmode)();
	(this->*inst.operate)();
}

void Cpu6502::reset() {
	uint8_t lo = bus->read(0xFFFC);
	uint8_t hi = bus->read(0xFFFD);

	acc = 0;
	x_ind = 0;
	y_ind = 0;
	PC = (hi << 8) | lo;
	SP = 0xFD;
	flag_n = 0;
	flag_v = 0;
	flag_1 = 0;
	flag_b = 0;
	flag_d = 0;
	flag_i = 1;
	flag_z = 0;
	flag_c = 0;
}

uint8_t Cpu6502::NOP() {
	return 0;
}

uint8_t Cpu6502::JMP() {
	PC = bus->read(target_addr);
	return 0;
}

uint8_t Cpu6502::BPL() {
	if (flag_n == 0) {
		uint8_t cycles = 1; // Branching takes at least 1 extra cycle
		
		if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
			cycles++;
		}

		PC = target_addr;
		return cycles;
	}

	return 0;
}

uint8_t Cpu6502::TXS() {
	SP = x_ind;
	return 0;
}

uint8_t Cpu6502::LDX() {
	x_ind = bus->read(target_addr);
	return 0;
}

uint8_t Cpu6502::STA() {
	bus->write(target_addr, acc);
	return 0;
}

uint8_t Cpu6502::STX() {
	bus->write(target_addr, x_ind);
	return 0;
}

uint8_t Cpu6502::STY() {
	bus->write(target_addr, y_ind);
	return 0;
}

uint8_t Cpu6502::SEI() {
	flag_i = 1;
	return 0;
}

uint8_t Cpu6502::CLD() {
	flag_d = 0;
	return 0;
}

uint8_t Cpu6502::BRK() {
	return 0;
}

uint8_t Cpu6502::CPY() {
	uint8_t M = bus->read(target_addr);
	uint8_t result = y_ind - M;

	// NVIB DIZC
	flag_c = y_ind >= M;
	flag_z = y_ind == M;
	flag_n = (result & 0x80); // 7th bit

	return 0;
}

uint8_t Cpu6502::LDA() {
	acc = bus->read(target_addr);
	flag_z = (acc == 0);
	flag_n = (acc & 0x80);
	return 0;
}

// ----- Addressing Modes -----

// Implicit
uint8_t Cpu6502::IMP() {
	return 0;
}

// Accumulator
uint8_t Cpu6502::ACC() {
	return 0;
}

// Immediate
uint8_t Cpu6502::IMM() {
	// Uses the 8-bit operand itself as the value for the operation, rather than fetching a value from a memory address. 
	target_addr = PC;
	PC++;
	return 0;
}

// Zero Page
uint8_t Cpu6502::ZP0() {
	uint8_t addr = bus->read(PC);
	PC++;
	target_addr = (uint16_t)addr;
	return 0;
}

// Zero Page Indexed (x)
uint8_t Cpu6502::ZPX() {
	uint8_t addr = bus->read(PC);
	PC++;
	target_addr = (uint16_t)(uint8_t)(addr + x_ind);
	return 0;
}

// Zero Page Indexed (y)
uint8_t Cpu6502::ZPY() {
	uint8_t addr = bus->read(PC);
	PC++;
	target_addr = (uint16_t)(uint8_t)(addr + y_ind);
	return 0;
}

// Absolute
uint8_t Cpu6502::AB0() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = bus->read(PC);
	PC++;
	uint8_t hi = bus->read(PC);
	PC++;

	target_addr = (hi << 8) | lo;
	return 0;
}

// Absolute Indexed (x)
uint8_t Cpu6502::ABX() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = bus->read(PC);
	PC++;
	uint8_t hi = bus->read(PC);
	PC++;

	uint16_t addr = (hi << 8) | lo;

	target_addr = addr + x_ind;

	if ((addr & 0xFF00) != (target_addr & 0xFF00)) return 1; // Oops cycle

	return 0;
}

// Absolute Indexed (y)
uint8_t Cpu6502::ABY() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = bus->read(PC);
	PC++;
	uint8_t hi = bus->read(PC);
	PC++;

	uint16_t addr = (hi << 8) | lo;

	target_addr = addr + y_ind;

	if ((addr & 0xFF00) != (target_addr & 0xFF00)) return 1; // Oops cycle

	return 0;
}

// Relative
uint8_t Cpu6502::REL() {
	uint8_t u_offset = bus->read(PC);
	int8_t offset = (int8_t)(u_offset);
	PC++;

	target_addr = PC + offset;

	return 0;
}

// Indirect
uint8_t Cpu6502::IND() {
	uint8_t lo = bus->read(PC);
	PC++;
	uint8_t hi = bus->read(PC);
	PC++;

	uint16_t ptr = (hi << 8) | lo;

	uint16_t ptr_hi_addr;

	if ((ptr & 0xFF) == 0xFF) { // Make sure to implement the CPU bug
		// If this 2-byte variable has an address ending in $FF and thus crosses a page
		// then the CPU fails to increment the page when reading the second byte
		// For example, JMP ($03FF) reads $03FF and $0300 instead of $0400. 
		ptr_hi_addr = ptr & 0xFF00;
	}
	else {
		ptr_hi_addr = ptr + 1;
	}

	uint16_t target_lo = bus->read(ptr);
	uint16_t target_hi = bus->read(ptr_hi_addr);


	target_addr = (target_hi << 8) | target_lo;

	return 0;
}

// Indirect Indexed (x)
uint8_t Cpu6502::IZX() {
	uint8_t base = bus->read(PC++);
	uint8_t lo_addr = (uint8_t)(base + x_ind);
	uint8_t hi_addr = (uint8_t)(base + x_ind + 1);

	uint8_t lo = bus->read((uint16_t)lo_addr);
	uint8_t hi = bus->read((uint16_t)hi_addr);

	target_addr = (hi << 8) | lo;

	return 0;
}

// Indirect Indexed (y)
uint8_t Cpu6502::IZY() {
	uint8_t ial = bus->read(PC++);

	uint8_t lo = bus->read((uint16_t)ial);
	uint8_t hi = bus->read((uint16_t)(uint8_t)(ial + 1)); // Apply (uint8_t) for wrapping

	uint16_t base_addr = (hi << 8) | lo;
	target_addr = base_addr + y_ind;

	if ((base_addr & 0xFF00) != (target_addr & 0xFF00)) return 1; // Oops cycle

	return 0;
}