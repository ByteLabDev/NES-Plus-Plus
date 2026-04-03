#pragma once
#include <cstdint>
#include <fstream>
#include <cstring>

class Nes;

class Cpu6502 {
	public:
		Cpu6502(Nes* nesPtr);

		// Thanks to this repo: https://github.com/tucna/NES-emulator/blob/master/sources/CPU.cpp
		struct Instruction {
			std::string name;
			uint8_t(Cpu6502::* operate)(void);
			uint8_t(Cpu6502::* addrmode)(void);
			uint8_t cycles;
		};

		void init();
		uint8_t step();
		
		// ----- Interrupts -----
		void reset();
		void IRQ();
		void NMI();
		void handle_nmi();
	private:
		uint16_t PC;    // Program Counter | Shows up to 64 kb (2^16 bytes) memory
		uint8_t SP;     // Stack Pointer

		// ----- Registers -----
		uint8_t acc;    // Accumulator
		uint8_t x_ind;  // X Index
		uint8_t y_ind;  // Y Index
		uint8_t stat;   // Status

		uint16_t target_addr;

		// ----- Flags -----
		union FLAGS {
			struct {
				uint8_t c : 1;    // Carry
				uint8_t z : 1;    // Zero
				uint8_t i : 1;    // Interrupt Disable
				uint8_t d : 1;    // Decimal
				uint8_t b : 1;    // Break
				uint8_t u : 1;    // Unused (always 1)
				uint8_t v : 1;    // Overflow
				uint8_t n : 1;    // Negative

			};
			uint8_t reg;
		};

		FLAGS flags{ 0b00100000 };

		Nes* nes;

		bool nmi_pending = false;

		Instruction instruction_lookup[256];

		void set_nz(uint8_t val); // Helper function

		// ----- Opcodes -----
		uint8_t NOP();
		uint8_t LDA();
		uint8_t LDX();
		uint8_t LDY();
		uint8_t STA();
		uint8_t STX();
		uint8_t STY();
		uint8_t SEI();
		uint8_t JMP();
		uint8_t BNE();
		uint8_t BEQ();
		uint8_t CEC();
		uint8_t CLC();
		uint8_t CLD();
		uint8_t ADC();
		uint8_t INX();
		uint8_t DEX();
		uint8_t BRK();
		uint8_t CPY();
		uint8_t TXS();
		uint8_t BPL();

		// ----- Addressing Modes -----
		uint8_t IMP();	uint8_t ACC();	uint8_t IMM();
		uint8_t ZP0();	uint8_t ZPX();	uint8_t ZPY();
		uint8_t AB0();	uint8_t ABX();	uint8_t ABY();
		uint8_t REL();
		uint8_t IND();	uint8_t IZX();	uint8_t IZY();
};