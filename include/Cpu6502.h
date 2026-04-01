#pragma once
#include <cstdint>
#include <fstream>
#include <cstring>
#include "Bus.h"

class Cpu6502 {
	public:
		Cpu6502(Bus* busPtr);

		// Thanks to this repo: https://github.com/tucna/NES-emulator/blob/master/sources/CPU.h
		struct Instruction {
			std::string name;
			uint8_t(Cpu6502::* operate)(void);
			uint8_t(Cpu6502::* addrmode)(void);
			uint8_t cycles;
		};

		void init();
		void step();
		void reset();

		// ----- Opcodes -----
		uint8_t NOP();
		uint8_t LDA();
		uint8_t LDX();
		uint8_t LDY();
		uint8_t STA();
		uint8_t STX();
		uint8_t STY();
		uint8_t JMP();
		uint8_t BNE();
		uint8_t BEQ();
		uint8_t CEC();
		uint8_t CLC();
		uint8_t ADC();
		uint8_t INX();
		uint8_t DEX();
		uint8_t BRK();

		// ----- Addressing Modes -----
		uint8_t IMP();
	private:
		uint16_t PC;    // Program Counter | Shows up to 64 kb (2^16 bytes) memory
		uint8_t SP;     // Stack Pointer

		// ----- Registers -----
		uint8_t acc;    // Accumulator
		uint8_t x_ind;  // X Index
		uint8_t y_ind;  // Y Index
		uint8_t stat;   // Status

		// ----- Modules -----
		Bus* bus;

		Instruction instruction_lookup[256];
};