// include/Cpu6502.h

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
		void handle_irq();
		void handle_nmi();
	private:
		Nes* nes;

		uint8_t opcode; // Current opcode

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
		bool irq_pending = false;
		bool nmi_pending = false;

		std::array<Instruction, 256> il; // Instruction lookup

		// ----- Helpers -----
		void set_nz(uint8_t val);
		void stack_push(uint8_t data);
		uint8_t stack_pop();
		uint8_t branch();

		// ----- Opcodes -----

		// Access
		uint8_t LDA(); uint8_t STA(); uint8_t LDX(); uint8_t STX(); uint8_t LDY(); uint8_t STY();

		// Transfer
		uint8_t TAX(); uint8_t TXA(); uint8_t TAY(); uint8_t TYA();

		// Arithmetic
		uint8_t ADC(); uint8_t SBC(); uint8_t INC(); uint8_t DEC(); uint8_t INX(); uint8_t DEX(); uint8_t INY(); uint8_t DEY();

		// Shift
		uint8_t ASL(); uint8_t LSR(); uint8_t ROL(); uint8_t ROR();

		// Bitwise
		uint8_t AND(); uint8_t ORA(); uint8_t EOR(); uint8_t BIT();

		// Compare
		uint8_t CMP(); uint8_t CPX(); uint8_t CPY();

		// Branch
		uint8_t BCC(); uint8_t BCS(); uint8_t BEQ(); uint8_t BNE(); uint8_t BPL(); uint8_t BMI(); uint8_t BVC(); uint8_t BVS();

		// Jump
		uint8_t JMP(); uint8_t JSR(); uint8_t RTS(); uint8_t BRK(); uint8_t RTI();

		// Stack
		uint8_t PHA(); uint8_t PLA(); uint8_t PHP(); uint8_t PLP(); uint8_t TXS(); uint8_t TSX();

		// Flags
		uint8_t CLC(); uint8_t SEC(); uint8_t CLI(); uint8_t SEI(); uint8_t CLD(); uint8_t SED(); uint8_t CLV();

		// Other
		uint8_t NOP();

		// ----- Illegal Opcodes -----

		// Combined Operations
		uint8_t ALR(); uint8_t ANC(); uint8_t ARR(); uint8_t AXS(); uint8_t LAX(); uint8_t SAX();

		// Read-Modify-Write Instructions
		uint8_t DCP(); uint8_t ISC(); uint8_t RLA(); uint8_t RRA(); uint8_t SLO(); uint8_t SRE();

		// ----- Addressing Modes -----
		uint8_t IMP();	uint8_t ACC();	uint8_t IMM();
		uint8_t ZP0();	uint8_t ZPX();	uint8_t ZPY();
		uint8_t AB0();	uint8_t ABX();	uint8_t ABY();
		uint8_t REL();
		uint8_t IND();	uint8_t IZX();	uint8_t IZY();
};