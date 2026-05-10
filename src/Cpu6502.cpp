// src/Cpu6502.cpp

// The NES CPU core is based on the 6502 processor and runs at approximately 1.79 MHz (1.66 MHz in a PAL NES).
// https://www.nesdev.org/wiki/CPU
// https://www.masswerk.at/6502/6502_instruction_set.html

#include "Nes.h"
#include "Cpu6502.h"
#include "Bus.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string> 
#include <algorithm>
#include <array>

Cpu6502::Cpu6502(Nes* nesPtr) {
	nes = nesPtr;

	// TEMP: Fill the instructions with blank data
	for (int i = 0; i < 256; i++) {
		il[i] = { "UNK", &Cpu6502::NOP, &Cpu6502::IMP, 2 };
	}

	// Access
	il[0xA9] = { "LDA", &Cpu6502::LDA, &Cpu6502::IMM, 2 }; il[0xA5] = { "LDA", &Cpu6502::LDA, &Cpu6502::ZP0, 3 }; il[0xB5] = { "LDA", &Cpu6502::LDA, &Cpu6502::ZPX, 4 };
	il[0xAD] = { "LDA", &Cpu6502::LDA, &Cpu6502::AB0, 4 }; il[0xBD] = { "LDA", &Cpu6502::LDA, &Cpu6502::ABX, 4 }; il[0xB9] = { "LDA", &Cpu6502::LDA, &Cpu6502::ABY, 4 };
	il[0xA1] = { "LDA", &Cpu6502::LDA, &Cpu6502::IZX, 6 }; il[0xB1] = { "LDA", &Cpu6502::LDA, &Cpu6502::IZY, 5 }; il[0x85] = { "STA", &Cpu6502::STA, &Cpu6502::ZP0, 3 };
	il[0x95] = { "STA", &Cpu6502::STA, &Cpu6502::ZPX, 4 }; il[0x8D] = { "STA", &Cpu6502::STA, &Cpu6502::AB0, 4 }; il[0x9D] = { "STA", &Cpu6502::STA, &Cpu6502::ABX, 5 };
	il[0x99] = { "STA", &Cpu6502::STA, &Cpu6502::ABY, 5 }; il[0x81] = { "STA", &Cpu6502::STA, &Cpu6502::IZX, 6 }; il[0x91] = { "STA", &Cpu6502::STA, &Cpu6502::IZY, 6 };
	il[0xA2] = { "LDX", &Cpu6502::LDX, &Cpu6502::IMM, 2 }; il[0xA6] = { "LDX", &Cpu6502::LDX, &Cpu6502::ZP0, 3 }; il[0xB6] = { "LDX", &Cpu6502::LDX, &Cpu6502::ZPY, 4 };
	il[0xAE] = { "LDX", &Cpu6502::LDX, &Cpu6502::AB0, 4 }; il[0xBE] = { "LDX", &Cpu6502::LDX, &Cpu6502::ABY, 4 }; il[0x86] = { "STX", &Cpu6502::STX, &Cpu6502::ZP0, 3 };
	il[0x96] = { "STX", &Cpu6502::STX, &Cpu6502::ZPY, 4 }; il[0x8E] = { "STX", &Cpu6502::STX, &Cpu6502::AB0, 4 }; il[0xA0] = { "LDY", &Cpu6502::LDY, &Cpu6502::IMM, 2 };
	il[0xA4] = { "LDY", &Cpu6502::LDY, &Cpu6502::ZP0, 3 }; il[0xB4] = { "LDY", &Cpu6502::LDY, &Cpu6502::ZPX, 4 }; il[0xAC] = { "LDY", &Cpu6502::LDY, &Cpu6502::AB0, 4 };
	il[0xBC] = { "LDY", &Cpu6502::LDY, &Cpu6502::ABX, 4 }; il[0x84] = { "STY", &Cpu6502::STY, &Cpu6502::ZP0, 3 }; il[0x94] = { "STY", &Cpu6502::STY, &Cpu6502::ZPX, 4 };
	il[0x8C] = { "STY", &Cpu6502::STY, &Cpu6502::AB0, 4 };

	// Transfer
	il[0xAA] = { "TAX", &Cpu6502::TAX, &Cpu6502::IMP, 2 }; il[0x8A] = { "TXA", &Cpu6502::TXA, &Cpu6502::IMP, 2 }; il[0xA8] = { "TAY", &Cpu6502::TAY, &Cpu6502::IMP, 2 };
	il[0x98] = { "TYA", &Cpu6502::TYA, &Cpu6502::IMP, 2 };

	// Arithmetic
	il[0x69] = { "ADC", &Cpu6502::ADC, &Cpu6502::IMM, 2 }; il[0x65] = { "ADC", &Cpu6502::ADC, &Cpu6502::ZP0, 3 }; il[0x75] = { "ADC", &Cpu6502::ADC, &Cpu6502::ZPX, 4 };
	il[0x6D] = { "ADC", &Cpu6502::ADC, &Cpu6502::AB0, 4 }; il[0x7D] = { "ADC", &Cpu6502::ADC, &Cpu6502::ABX, 4 }; il[0x79] = { "ADC", &Cpu6502::ADC, &Cpu6502::ABY, 4 };
	il[0x61] = { "ADC", &Cpu6502::ADC, &Cpu6502::IZX, 6 }; il[0x71] = { "ADC", &Cpu6502::ADC, &Cpu6502::IZY, 5 }; il[0xE9] = { "SBC", &Cpu6502::SBC, &Cpu6502::IMM, 2 };
	il[0xE5] = { "SBC", &Cpu6502::SBC, &Cpu6502::ZP0, 3 }; il[0xF5] = { "SBC", &Cpu6502::SBC, &Cpu6502::ZPX, 4 }; il[0xED] = { "SBC", &Cpu6502::SBC, &Cpu6502::AB0, 4 };
	il[0xFD] = { "SBC", &Cpu6502::SBC, &Cpu6502::ABX, 4 }; il[0xF9] = { "SBC", &Cpu6502::SBC, &Cpu6502::ABY, 4 }; il[0xE1] = { "SBC", &Cpu6502::SBC, &Cpu6502::IZX, 6 };
	il[0xF1] = { "SBC", &Cpu6502::SBC, &Cpu6502::IZY, 5 }; il[0xE6] = { "INC", &Cpu6502::INC, &Cpu6502::ZP0, 5 }; il[0xF6] = { "INC", &Cpu6502::INC, &Cpu6502::ZPX, 6 };
	il[0xEE] = { "INC", &Cpu6502::INC, &Cpu6502::AB0, 6 }; il[0xFE] = { "INC", &Cpu6502::INC, &Cpu6502::ABX, 7 }; il[0xC6] = { "DEC", &Cpu6502::DEC, &Cpu6502::ZP0, 5 };
	il[0xD6] = { "DEC", &Cpu6502::DEC, &Cpu6502::ZPX, 6 }; il[0xCE] = { "DEC", &Cpu6502::DEC, &Cpu6502::AB0, 6 }; il[0xDE] = { "DEC", &Cpu6502::DEC, &Cpu6502::ABX, 7 };
	il[0xE8] = { "INX", &Cpu6502::INX, &Cpu6502::IMP, 2 }; il[0xCA] = { "DEX", &Cpu6502::DEX, &Cpu6502::IMP, 2 }; il[0xC8] = { "INY", &Cpu6502::INY, &Cpu6502::IMP, 2 };
	il[0x88] = { "DEY", &Cpu6502::DEY, &Cpu6502::IMP, 2 };

	// Shift
	il[0x0A] = { "ASL", &Cpu6502::ASL, &Cpu6502::ACC, 2 }; il[0x06] = { "ASL", &Cpu6502::ASL, &Cpu6502::ZP0, 5 }; il[0x16] = { "ASL", &Cpu6502::ASL, &Cpu6502::ZPX, 6 };
	il[0x0E] = { "ASL", &Cpu6502::ASL, &Cpu6502::AB0, 6 }; il[0x1E] = { "ASL", &Cpu6502::ASL, &Cpu6502::ABX, 7 };
	il[0x4A] = { "LSR", &Cpu6502::LSR, &Cpu6502::ACC, 2 }; il[0x46] = { "LSR", &Cpu6502::LSR, &Cpu6502::ZP0, 5 }; il[0x56] = { "LSR", &Cpu6502::LSR, &Cpu6502::ZPX, 6 };
	il[0x4E] = { "LSR", &Cpu6502::LSR, &Cpu6502::AB0, 6 }; il[0x5E] = { "LSR", &Cpu6502::LSR, &Cpu6502::ABX, 7 };
	il[0x2A] = { "ROL", &Cpu6502::ROL, &Cpu6502::ACC, 2 }; il[0x26] = { "ROL", &Cpu6502::ROL, &Cpu6502::ZP0, 5 }; il[0x36] = { "ROL", &Cpu6502::ROL, &Cpu6502::ZPX, 6 };
	il[0x2E] = { "ROL", &Cpu6502::ROL, &Cpu6502::AB0, 6 }; il[0x3E] = { "ROL", &Cpu6502::ROL, &Cpu6502::ABX, 7 };
	il[0x6A] = { "ROR", &Cpu6502::ROR, &Cpu6502::ACC, 2 }; il[0x66] = { "ROR", &Cpu6502::ROR, &Cpu6502::ZP0, 5 }; il[0x76] = { "ROR", &Cpu6502::ROR, &Cpu6502::ZPX, 6 };
	il[0x6E] = { "ROR", &Cpu6502::ROR, &Cpu6502::AB0, 6 }; il[0x7E] = { "ROR", &Cpu6502::ROR, &Cpu6502::ABX, 7 };

	// Bitwise
	il[0x29] = { "AND", &Cpu6502::AND, &Cpu6502::IMM, 2 }; il[0x25] = { "AND", &Cpu6502::AND, &Cpu6502::ZP0, 3 }; il[0x35] = { "AND", &Cpu6502::AND, &Cpu6502::ZPX, 4 };
	il[0x2D] = { "AND", &Cpu6502::AND, &Cpu6502::AB0, 4 }; il[0x3D] = { "AND", &Cpu6502::AND, &Cpu6502::ABX, 4 }; il[0x39] = { "AND", &Cpu6502::AND, &Cpu6502::ABY, 4 };
	il[0x21] = { "AND", &Cpu6502::AND, &Cpu6502::IZX, 6 }; il[0x31] = { "AND", &Cpu6502::AND, &Cpu6502::IZY, 5 }; il[0x09] = { "ORA", &Cpu6502::ORA, &Cpu6502::IMM, 2 };
	il[0x05] = { "ORA", &Cpu6502::ORA, &Cpu6502::ZP0, 3 }; il[0x15] = { "ORA", &Cpu6502::ORA, &Cpu6502::ZPX, 4 }; il[0x0D] = { "ORA", &Cpu6502::ORA, &Cpu6502::AB0, 4 };
	il[0x1D] = { "ORA", &Cpu6502::ORA, &Cpu6502::ABX, 4 }; il[0x19] = { "ORA", &Cpu6502::ORA, &Cpu6502::ABY, 4 }; il[0x01] = { "ORA", &Cpu6502::ORA, &Cpu6502::IZX, 6 };
	il[0x11] = { "ORA", &Cpu6502::ORA, &Cpu6502::IZY, 5 }; il[0x49] = { "EOR", &Cpu6502::EOR, &Cpu6502::IMM, 2 }; il[0x45] = { "EOR", &Cpu6502::EOR, &Cpu6502::ZP0, 3 };
	il[0x55] = { "EOR", &Cpu6502::EOR, &Cpu6502::ZPX, 4 }; il[0x4D] = { "EOR", &Cpu6502::EOR, &Cpu6502::AB0, 4 }; il[0x5D] = { "EOR", &Cpu6502::EOR, &Cpu6502::ABX, 4 };
	il[0x59] = { "EOR", &Cpu6502::EOR, &Cpu6502::ABY, 4 }; il[0x41] = { "EOR", &Cpu6502::EOR, &Cpu6502::IZX, 6 }; il[0x51] = { "EOR", &Cpu6502::EOR, &Cpu6502::IZY, 5 };
	il[0x24] = { "BIT", &Cpu6502::BIT, &Cpu6502::ZP0, 3 }; il[0x2C] = { "BIT", &Cpu6502::BIT, &Cpu6502::AB0, 4 };

	// Compare
	il[0xC9] = { "CMP", &Cpu6502::CMP, &Cpu6502::IMM, 2 }; il[0xC5] = { "CMP", &Cpu6502::CMP, &Cpu6502::ZP0, 3 }; il[0xD5] = { "CMP", &Cpu6502::CMP, &Cpu6502::ZPX, 4 };
	il[0xCD] = { "CMP", &Cpu6502::CMP, &Cpu6502::AB0, 4 }; il[0xDD] = { "CMP", &Cpu6502::CMP, &Cpu6502::ABX, 4 }; il[0xD9] = { "CMP", &Cpu6502::CMP, &Cpu6502::ABY, 4 };
	il[0xC1] = { "CMP", &Cpu6502::CMP, &Cpu6502::IZX, 6 }; il[0xD1] = { "CMP", &Cpu6502::CMP, &Cpu6502::IZY, 5 };
	il[0xE0] = { "CPX", &Cpu6502::CPX, &Cpu6502::IMM, 2 }; il[0xE4] = { "CPX", &Cpu6502::CPX, &Cpu6502::ZP0, 3 }; il[0xEC] = { "CPX", &Cpu6502::CPX, &Cpu6502::AB0, 4 };
	il[0xC0] = { "CPY", &Cpu6502::CPY, &Cpu6502::IMM, 2 }; il[0xC4] = { "CPY", &Cpu6502::CPY, &Cpu6502::ZP0, 3 }; il[0xCC] = { "CPY", &Cpu6502::CPY, &Cpu6502::AB0, 4 };

	// Branch
	il[0x90] = { "BCC", &Cpu6502::BCC, &Cpu6502::REL, 2 }; il[0xB0] = { "BCS", &Cpu6502::BCS, &Cpu6502::REL, 2 }; il[0xF0] = { "BEQ", &Cpu6502::BEQ, &Cpu6502::REL, 2 };
	il[0xD0] = { "BNE", &Cpu6502::BNE, &Cpu6502::REL, 2 }; il[0x10] = { "BPL", &Cpu6502::BPL, &Cpu6502::REL, 2 }; il[0x30] = { "BMI", &Cpu6502::BMI, &Cpu6502::REL, 2 };
	il[0x50] = { "BVC", &Cpu6502::BVC, &Cpu6502::REL, 2 }; il[0x70] = { "BVS", &Cpu6502::BVS, &Cpu6502::REL, 2 };

	// Jump
	il[0x4C] = { "JMP", &Cpu6502::JMP, &Cpu6502::AB0, 3 }; il[0x6C] = { "JMP", &Cpu6502::JMP, &Cpu6502::IND, 5 }; il[0x20] = { "JSR", &Cpu6502::JSR, &Cpu6502::AB0, 6 };
	il[0x60] = { "RTS", &Cpu6502::RTS, &Cpu6502::IMP, 6 }; il[0x00] = { "BRK", &Cpu6502::BRK, &Cpu6502::IMP, 7 }; il[0x40] = { "RTI", &Cpu6502::RTI, &Cpu6502::IMP, 6 };

	// Stack
	il[0x48] = { "PHA", &Cpu6502::PHA, &Cpu6502::IMP, 3 }; il[0x08] = { "PHP", &Cpu6502::PHP, &Cpu6502::IMP, 3 }; il[0x68] = { "PLA", &Cpu6502::PLA, &Cpu6502::IMP, 4 };
	il[0x28] = { "PLP", &Cpu6502::PLP, &Cpu6502::IMP, 4 }; il[0x9A] = { "TXS", &Cpu6502::TXS, &Cpu6502::IMP, 2 }; il[0xBA] = { "TSX", &Cpu6502::TSX, &Cpu6502::IMP, 2 };

	// Flags
	il[0x18] = { "CLC", &Cpu6502::CLC, &Cpu6502::IMP, 2 }; il[0x38] = { "SEC", &Cpu6502::SEC, &Cpu6502::IMP, 2 }; il[0x58] = { "CLI", &Cpu6502::CLI, &Cpu6502::IMP, 2 };
	il[0x78] = { "SEI", &Cpu6502::SEI, &Cpu6502::IMP, 2 }; il[0xD8] = { "CLD", &Cpu6502::CLD, &Cpu6502::IMP, 2 }; il[0xF8] = { "SED", &Cpu6502::SED, &Cpu6502::IMP, 2 };
	il[0xB8] = { "CLV", &Cpu6502::CLV, &Cpu6502::IMP, 2 };

	// Other
	il[0xEA] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 };

	// ----- Illegal Opcodes -----

	// Combined operations
	il[0x4B] = { "ALR", &Cpu6502::ALR, &Cpu6502::IMM, 2 }; il[0x0B] = { "ANC", &Cpu6502::ANC, &Cpu6502::IMM, 2 }; il[0x2B] = { "ANC", &Cpu6502::ANC, &Cpu6502::IMM, 2 };
	il[0x6B] = { "ARR", &Cpu6502::ARR, &Cpu6502::IMM, 2 }; il[0xCB] = { "AXS", &Cpu6502::AXS, &Cpu6502::IMM, 2 }; il[0xA3] = { "LAX", &Cpu6502::LAX, &Cpu6502::IZX, 6 };
	il[0xA7] = { "LAX", &Cpu6502::LAX, &Cpu6502::ZP0, 3 }; il[0xAF] = { "LAX", &Cpu6502::LAX, &Cpu6502::AB0, 4 }; il[0xB3] = { "LAX", &Cpu6502::LAX, &Cpu6502::IZY, 5 };
	il[0xB7] = { "LAX", &Cpu6502::LAX, &Cpu6502::ZPY, 4 }; il[0xBF] = { "LAX", &Cpu6502::LAX, &Cpu6502::ABY, 4 }; il[0x83] = { "SAX", &Cpu6502::SAX, &Cpu6502::IZX, 6 };
	il[0x87] = { "SAX", &Cpu6502::SAX, &Cpu6502::ZP0, 3 }; il[0x8F] = { "SAX", &Cpu6502::SAX, &Cpu6502::AB0, 4 }; il[0x97] = { "SAX", &Cpu6502::SAX, &Cpu6502::ZPY, 4 };

	// Read-Modify-Write Instructions
	il[0xC3] = { "DCP", &Cpu6502::DCP, &Cpu6502::IZX, 8 }; il[0xC7] = { "DCP", &Cpu6502::DCP, &Cpu6502::ZP0, 5 }; il[0xCF] = { "DCP", &Cpu6502::DCP, &Cpu6502::AB0, 6 };
	il[0xD3] = { "DCP", &Cpu6502::DCP, &Cpu6502::IZY, 8 }; il[0xD7] = { "DCP", &Cpu6502::DCP, &Cpu6502::ZPX, 6 }; il[0xDB] = { "DCP", &Cpu6502::DCP, &Cpu6502::ABY, 7 };
	il[0xDF] = { "DCP", &Cpu6502::DCP, &Cpu6502::ABX, 7 }; il[0xE3] = { "ISC", &Cpu6502::ISC, &Cpu6502::IZX, 8 }; il[0xE7] = { "ISC", &Cpu6502::ISC, &Cpu6502::ZP0, 5 };
	il[0xEF] = { "ISC", &Cpu6502::ISC, &Cpu6502::AB0, 6 }; il[0xF3] = { "ISC", &Cpu6502::ISC, &Cpu6502::IZY, 8 }; il[0xF7] = { "ISC", &Cpu6502::ISC, &Cpu6502::ZPX, 6 };
	il[0xFB] = { "ISC", &Cpu6502::ISC, &Cpu6502::ABY, 7 }; il[0xFF] = { "ISC", &Cpu6502::ISC, &Cpu6502::ABX, 7 }; il[0x23] = { "RLA", &Cpu6502::RLA, &Cpu6502::IZX, 8 };
	il[0x27] = { "RLA", &Cpu6502::RLA, &Cpu6502::ZP0, 5 }; il[0x2F] = { "RLA", &Cpu6502::RLA, &Cpu6502::AB0, 6 }; il[0x33] = { "RLA", &Cpu6502::RLA, &Cpu6502::IZY, 8 };
	il[0x37] = { "RLA", &Cpu6502::RLA, &Cpu6502::ZPX, 6 }; il[0x3B] = { "RLA", &Cpu6502::RLA, &Cpu6502::ABY, 7 }; il[0x3F] = { "RLA", &Cpu6502::RLA, &Cpu6502::ABX, 7 };
	il[0x63] = { "RRA", &Cpu6502::RRA, &Cpu6502::IZX, 8 }; il[0x67] = { "RRA", &Cpu6502::RRA, &Cpu6502::ZP0, 5 }; il[0x6F] = { "RRA", &Cpu6502::RRA, &Cpu6502::AB0, 6 };
	il[0x73] = { "RRA", &Cpu6502::RRA, &Cpu6502::IZY, 8 }; il[0x77] = { "RRA", &Cpu6502::RRA, &Cpu6502::ZPX, 6 }; il[0x7B] = { "RRA", &Cpu6502::RRA, &Cpu6502::ABY, 7 };
	il[0x7F] = { "RRA", &Cpu6502::RRA, &Cpu6502::ABX, 7 }; il[0x03] = { "SLO", &Cpu6502::SLO, &Cpu6502::IZX, 8 }; il[0x07] = { "SLO", &Cpu6502::SLO, &Cpu6502::ZP0, 5 };
	il[0x0F] = { "SLO", &Cpu6502::SLO, &Cpu6502::AB0, 6 }; il[0x13] = { "SLO", &Cpu6502::SLO, &Cpu6502::IZY, 8 }; il[0x17] = { "SLO", &Cpu6502::SLO, &Cpu6502::ZPX, 6 };
	il[0x1B] = { "SLO", &Cpu6502::SLO, &Cpu6502::ABY, 7 }; il[0x1F] = { "SLO", &Cpu6502::SLO, &Cpu6502::ABX, 7 }; il[0x43] = { "SRE", &Cpu6502::SRE, &Cpu6502::IZX, 8 };
	il[0x47] = { "SRE", &Cpu6502::SRE, &Cpu6502::ZP0, 5 }; il[0x4F] = { "SRE", &Cpu6502::SRE, &Cpu6502::AB0, 6 }; il[0x53] = { "SRE", &Cpu6502::SRE, &Cpu6502::IZY, 8 };
	il[0x57] = { "SRE", &Cpu6502::SRE, &Cpu6502::ZPX, 6 }; il[0x5B] = { "SRE", &Cpu6502::SRE, &Cpu6502::ABY, 7 }; il[0x5F] = { "SRE", &Cpu6502::SRE, &Cpu6502::ABX, 7 };

	// Duplicated instructions
	il[0xEB] = { "SBC", &Cpu6502::SBC, &Cpu6502::IMM, 2 };

	// Unimplemented addressing modes
	il[0x9E] = { "SHX", &Cpu6502::SHX, &Cpu6502::ABY, 5 };
	il[0x9C] = { "SHY", &Cpu6502::SHY, &Cpu6502::ABX, 5 };

	// NOPs
	il[0x1A] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 }; il[0x3A] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 }; il[0x5A] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 };
	il[0x7A] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 }; il[0xDA] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 }; il[0xEA] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 };
	il[0xFA] = { "NOP", &Cpu6502::NOP, &Cpu6502::IMP, 2 }; il[0x80] = { "SKB", &Cpu6502::NOP, &Cpu6502::IMM, 2 }; il[0x82] = { "SKB", &Cpu6502::NOP, &Cpu6502::IMM, 2 };
	il[0x89] = { "SKB", &Cpu6502::NOP, &Cpu6502::IMM, 2 }; il[0xC2] = { "SKB", &Cpu6502::NOP, &Cpu6502::IMM, 2 }; il[0xE2] = { "SKB", &Cpu6502::NOP, &Cpu6502::IMM, 2 };
	il[0x0C] = { "IGN", &Cpu6502::NOP, &Cpu6502::AB0, 4 }; il[0x1C] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 }; il[0x3C] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 };
	il[0x5C] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 }; il[0x7C] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 }; il[0xDC] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 };
	il[0xFC] = { "IGN", &Cpu6502::NOP, &Cpu6502::ABX, 4 }; il[0x04] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZP0, 3 }; il[0x44] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZP0, 3 };
	il[0x64] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZP0, 3 }; il[0x14] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 }; il[0x34] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 };
	il[0x54] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 }; il[0x74] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 }; il[0xD4] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 };
	il[0xF4] = { "IGN", &Cpu6502::NOP, &Cpu6502::ZPX, 4 }; il[0xD8] = { "CLD", &Cpu6502::CLD, &Cpu6502::IMP, 2 }; il[0xB8] = { "CLV", &Cpu6502::CLV, &Cpu6502::IMP, 2 };
	il[0xF8] = { "SED", &Cpu6502::SED, &Cpu6502::IMP, 2 };
}

uint8_t Cpu6502::step() {
	if (nmi_pending) {
		handle_nmi();
		return 7;
	}

	if (trigger_irq_next) {
		trigger_irq_next = false;
		handle_irq();
		return 7;
	}

	uint8_t old_i_flag = flags.i;

	opcode = nes->bus.read(PC++);
	Instruction& inst = il[opcode];

	if (inst.name == "UNK" || inst.addrmode == nullptr || inst.operate == nullptr) {
		std::cout << "Unimplemented Opcode: 0x" << std::hex << (int)opcode << std::endl;
		PC--;
		return 0;
	}

	uint8_t addr_cycles = (this->*inst.addrmode)();
	uint8_t op_cycles = (this->*inst.operate)();
	uint8_t total_cycles = inst.cycles + addr_cycles + op_cycles;

	uint8_t polled_i_flag = flags.i;
	if (inst.name == "CLI" || inst.name == "SEI" || inst.name == "PLP") {
		polled_i_flag = old_i_flag;
	}

	trigger_irq_next = (irq_pending && polled_i_flag == 0);

	return total_cycles;
}

void Cpu6502::soft_reset() {
	uint8_t lo = nes->bus.read(0xFFFC);
	uint8_t hi = nes->bus.read(0xFFFD);

	PC = (hi << 8) | lo;
	SP -= 3;

	flags.i = 1;
}

void Cpu6502::hard_reset() {
	uint8_t lo = nes->bus.read(0xFFFC);
	uint8_t hi = nes->bus.read(0xFFFD);

	PC = (hi << 8) | lo;
	SP = 0xFD;

	acc = 0;
	x_ind = 0;
	y_ind = 0;

	flags.reg = 0x34;

	opcode = 0;
	target_addr = 0;

	irq_pending = false;
	nmi_pending = false;
	trigger_irq_next = false;

	nes->bus.clear();
}

void Cpu6502::set_irq_line(bool state) {
	irq_pending = state;
}

void Cpu6502::NMI() {
	nmi_pending = true;
}

void Cpu6502::handle_irq() {
	// 1. Push Program Counter to stack
	stack_push((PC >> 8) & 0xFF);
	stack_push(PC & 0xFF);

	// 2. Push Status Register
	FLAGS status_to_push = flags;
	status_to_push.b = 0;
	status_to_push.u = 1;
	stack_push(status_to_push.reg);

	// 3. Set Interrupt Disable flag to prevent nested IRQs
	flags.i = 1;
	trigger_irq_next = false;

	// 4. Load PC from IRQ/BRK vector ($FFFE-$FFFF)
	uint8_t lo = nes->bus.read(0xFFFE);
	uint8_t hi = nes->bus.read(0xFFFF);
	PC = (hi << 8) | lo;

	// DO NOT set irq_pending = false here. 
}

void Cpu6502::handle_nmi() {
	// https://www.nesdev.org/wiki/NMI
	uint8_t hi = (uint8_t)((PC >> 8) & 0x00FF);
	uint8_t lo = (uint8_t)((PC) & 0x00FF);
	nes->bus.write(0x100 + SP--, hi);
	nes->bus.write(0x100 + SP--, lo);

	FLAGS new_status = flags;
	new_status.b = 0;
	new_status.u = 1;

	nes->bus.write(0x100 + SP--, new_status.reg);

	flags.i = 1;
	trigger_irq_next = false;

	// Jump to NMI
	uint8_t vector_lo = nes->bus.read(0xFFFA);
	uint8_t vector_hi = nes->bus.read(0xFFFB);

	PC = (vector_hi << 8) | vector_lo;

	nmi_pending = false;
}

void Cpu6502::set_nz(uint8_t val) {
	flags.z = (val == 0);
	flags.n = (val & 0x80) ? 1 : 0;
}

void Cpu6502::stack_push(uint8_t data) {
	nes->bus.write(0x0100 + SP, data);
	SP--;
}

uint8_t Cpu6502::stack_pop() {
	SP++;
	return nes->bus.read(0x0100 + SP);
}

// ----- Instructions -----

// Access
uint8_t Cpu6502::LDA() {
	acc = nes->bus.read(target_addr);
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::STA() {
	nes->bus.write(target_addr, acc);
	return 0;
}
uint8_t Cpu6502::LDX() {
	x_ind = nes->bus.read(target_addr);
	set_nz(x_ind);
	return 0;
}
uint8_t Cpu6502::STX() {
	nes->bus.write(target_addr, x_ind);
	return 0;
}
uint8_t Cpu6502::LDY() {
	y_ind = nes->bus.read(target_addr);
	set_nz(y_ind);
	return 0;
}
uint8_t Cpu6502::STY() {
	nes->bus.write(target_addr, y_ind);
	return 0;
}

// Transfer
uint8_t Cpu6502::TAX() {
	x_ind = acc;
	set_nz(x_ind);
	return 0;
}
uint8_t Cpu6502::TXA() {
	acc = x_ind;
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::TAY() {
	y_ind = acc;
	set_nz(y_ind);
	return 0;
}
uint8_t Cpu6502::TYA() {
	acc = y_ind;
	set_nz(acc);
	return 0;
}

// Arithmetic
uint8_t Cpu6502::ADC() {
	uint8_t mem = nes->bus.read(target_addr);
	uint16_t result = (uint16_t)acc + (uint16_t)mem + (uint16_t)flags.c;
	flags.c = (result > 0xFF) ? 1 : 0;
	flags.v = ((result ^ (uint16_t)acc) & (result ^ (uint16_t)mem) & 0x80) ? 1 : 0;
	set_nz(result);
	acc = result;
	return 0;
}
uint8_t Cpu6502::SBC() {
	uint8_t mem = nes->bus.read(target_addr);
	uint16_t flipped_mem = (uint16_t)mem ^ 0x00FF;
	uint16_t result = acc + flipped_mem + flags.c;
	flags.c = (result > 0xFF) ? 1 : 0;
	flags.v = ((result ^ (uint16_t)acc) & (result ^ flipped_mem) & 0x80) ? 1 : 0;
	set_nz(result);
	acc = (uint8_t)result;
	return 0;
}
uint8_t Cpu6502::INC() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);
	mem++;
	nes->bus.write(target_addr, mem);
	set_nz(mem);
	return 0;
}
uint8_t Cpu6502::DEC() {
    uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);
    mem--;
    nes->bus.write(target_addr, mem);
    set_nz(mem);
    return 0;
}
uint8_t Cpu6502::INX() {
	x_ind++;
	set_nz(x_ind);
	return 0;
}
uint8_t Cpu6502::DEX() {
	x_ind--;
	set_nz(x_ind);
	return 0;
}
uint8_t Cpu6502::INY() {
	y_ind++;
	set_nz(y_ind);
	return 0;
}
uint8_t Cpu6502::DEY() {
	y_ind--;
	set_nz(y_ind);
	return 0;
}

// Shift
uint8_t Cpu6502::ASL() {
	uint8_t value;
	bool is_acc = (il[opcode].addrmode == &Cpu6502::ACC);
	if (is_acc) {
		value = acc;
	} else {
		value = nes->bus.read(target_addr);
		nes->bus.write(target_addr, value);
	}
	flags.c = (value >> 7) & 0x01; // 7th bit
	value <<= 1;
	set_nz(value);

	if (il[opcode].addrmode == &Cpu6502::ACC) {
		acc = value;
	} else {
		nes->bus.write(target_addr, value);
	}
	return 0;
}
uint8_t Cpu6502::LSR() {
	uint8_t value;
	bool is_acc = (il[opcode].addrmode == &Cpu6502::ACC);
	if (is_acc) {
		value = acc;
	}
	else {
		value = nes->bus.read(target_addr);
		nes->bus.write(target_addr, value);
	}
	flags.c = value & 0x01; // Bit 0
	value >>= 1;
	flags.z = (value == 0) ? 1 : 0;
	flags.n = 0;

	if (il[opcode].addrmode == &Cpu6502::ACC) {
		acc = value;
	}
	else {
		nes->bus.write(target_addr, value);
	}
	return 0;
}
uint8_t Cpu6502::ROL() {
	uint8_t value;
	bool is_acc = (il[opcode].addrmode == &Cpu6502::ACC);
	if (is_acc) {
		value = acc;
	}
	else {
		value = nes->bus.read(target_addr);
		nes->bus.write(target_addr, value);
	}
	
	uint8_t old_carry = flags.c;
	flags.c = (value >> 7) & 0x01; // 7th bit

	value = (value << 1) | old_carry;
	set_nz(value);

	if (il[opcode].addrmode == &Cpu6502::ACC) {
		acc = value;
	}
	else {
		nes->bus.write(target_addr, value);
	}
	return 0;
}
uint8_t Cpu6502::ROR() {
	uint8_t value;
	bool is_acc = (il[opcode].addrmode == &Cpu6502::ACC);
	if (is_acc) {
		value = acc;
	}
	else {
		value = nes->bus.read(target_addr);
		nes->bus.write(target_addr, value);
	}

	uint8_t old_carry = flags.c;
	flags.c = value & 0x01; // Bit 0

	value = (value >> 1) | (old_carry << 7);
	set_nz(value);

	if (il[opcode].addrmode == &Cpu6502::ACC) {
		acc = value;
	}
	else {
		nes->bus.write(target_addr, value);
	}
	return 0;
}

// Bitwise
uint8_t Cpu6502::AND() {
	uint8_t mem = nes->bus.read(target_addr);
	acc = acc & mem;
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::ORA() {
	uint8_t mem = nes->bus.read(target_addr);
	acc = acc | mem;
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::EOR() {
	uint8_t mem = nes->bus.read(target_addr);
	acc = acc ^ mem;
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::BIT() {
	// BIT modifies flags, but does not change memory or registers.
	uint8_t mem = nes->bus.read(target_addr);
	uint8_t result = acc & mem;
	flags.z = (result == 0) ? 1 : 0;	// Result == 0
	flags.v = (mem >> 6) & 0x01;		// Memory bit 6
	flags.n = (mem >> 7) & 0x01;		// Memory bit 7
	return 0;
}

// Compare
uint8_t Cpu6502::CMP() {
	uint8_t mem = nes->bus.read(target_addr);
	uint8_t result = acc - mem;

	flags.c = (acc >= mem);
	flags.z = (acc == mem);
	flags.n = (result & 0x80) ? 1 : 0;

	return 0;
}
uint8_t Cpu6502::CPX() {
	uint8_t M = nes->bus.read(target_addr);
	uint8_t result = x_ind - M;

	// NVIB DIZC
	flags.c = x_ind >= M;
	set_nz(result);

	return 0;
}
uint8_t Cpu6502::CPY() {
	uint8_t M = nes->bus.read(target_addr);
	uint8_t result = y_ind - M;

	// NVIB DIZC
	flags.c = y_ind >= M;
	set_nz(result);

	return 0;
}

// Branch
uint8_t Cpu6502::branch() {
	uint8_t cycles = 1; // Branching takes at least 1 extra cycle

	if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
		cycles++; // Page crossed
	}
	PC = target_addr;

	return cycles;
}
uint8_t Cpu6502::BCC() {
	if (flags.c == 1) return 0;
	return branch();
}
uint8_t Cpu6502::BCS() {
	if (flags.c == 0) return 0;
	return branch();
}
uint8_t Cpu6502::BEQ() {
	if (flags.z == 0) return 0;
	return branch();
}
uint8_t Cpu6502::BNE() {
	if (flags.z == 1) return 0;
	return branch();
}
uint8_t Cpu6502::BPL() {
	if (flags.n != 0) return 0;
	return branch();
}
uint8_t Cpu6502::BMI() {
	if (flags.n == 0) return 0;
	return branch();
}
uint8_t Cpu6502::BVC() {
	if (flags.v == 1) return 0;
	return branch();
}
uint8_t Cpu6502::BVS() {
	if (flags.v == 0) return 0;
	return branch();
}

// Jump
uint8_t Cpu6502::JMP() {
	PC = target_addr;
	return 0;
}
uint8_t Cpu6502::JSR() {
	// Store the two bytes of the last instruction address to be executed on the stack, decrement SP by 2.
	uint16_t return_addr = PC - 1; // PC is 3 bytes ahead
	uint8_t hi = (return_addr & 0xFF00) >> 8;
	uint8_t lo = (return_addr & 0x00FF);

	stack_push(hi);
	stack_push(lo);

	PC = target_addr;
	return 0;
}
uint8_t Cpu6502::RTS() {
	// PC Lo --> PC Hi
	// Note: It seems 6502 User's Manual by Joseph J. Carr has an error claiming that the status register is also read. This should be ignored.
	uint16_t pc_lo = (uint16_t)stack_pop();
	uint16_t pc_hi = (uint16_t)stack_pop() << 8;
	PC = (pc_lo | pc_hi) + 1;

	return 0;
}
uint8_t Cpu6502::BRK() {
	PC++;
	stack_push((PC >> 8) & 0xFF);
	stack_push(PC & 0xFF);

	FLAGS status_to_push = flags;
	status_to_push.b = 1;
	status_to_push.u = 1;
	stack_push(status_to_push.reg);

	flags.i = 1;
	trigger_irq_next = false;

	uint8_t lo = nes->bus.read(0xFFFE);
	uint8_t hi = nes->bus.read(0xFFFF);
	PC = (hi << 8) | lo;
	return 0;
}
uint8_t Cpu6502::RTI() {
	// Status register -> PC Lo --> PC Hi
	flags.reg = stack_pop();
	uint16_t pc_lo = (uint16_t)stack_pop();
	uint16_t pc_hi = (uint16_t)stack_pop() << 8;
	PC = pc_lo | pc_hi;

	return 0;
}

// Stack
uint8_t Cpu6502::PHA() {
	stack_push(acc);
	return 0;
}
uint8_t Cpu6502::PLA() {
	acc = stack_pop();
	set_nz(acc);
	return 0;
}
uint8_t Cpu6502::PHP() {
	FLAGS temp_flags = flags;
	temp_flags.b = 1;
	temp_flags.u = 1;
	stack_push(temp_flags.reg);
	return 0;
}
uint8_t Cpu6502::PLP() {
	flags.reg = stack_pop();
	return 0;
}
uint8_t Cpu6502::TXS() {
	SP = x_ind;
	return 0;
}
uint8_t Cpu6502::TSX() {
	x_ind = SP;
	set_nz(x_ind);
	return 0;
}

// Flags
uint8_t Cpu6502::CLC() {
	flags.c = 0;
	return 0;
}
uint8_t Cpu6502::SEC() {
	flags.c = 1;
	return 0;
}
uint8_t Cpu6502::CLI() {
	flags.i = 0;
	return 0;
}
uint8_t Cpu6502::SEI() {
	flags.i = 1;
	return 0;
}
uint8_t Cpu6502::CLD() {
	flags.d = 0;
	return 0;
}
uint8_t Cpu6502::SED() {
	flags.d = 1;
	return 0;
}
uint8_t Cpu6502::CLV() {
	flags.v = 0;
	return 0;
}

// Other
uint8_t Cpu6502::NOP() {
	return 0;
}

// ----- Illegal Opcodes -----

// Combined Operations

uint8_t Cpu6502::ALR() {
	uint8_t mem = nes->bus.read(target_addr);
	acc &= mem;

	flags.c = acc & 0x01;
	acc >>= 1;

	set_nz(acc);

	return 0;
}
uint8_t Cpu6502::ANC() {
	uint8_t mem = nes->bus.read(target_addr);
	acc = acc & mem;
	set_nz(acc);
	flags.c = flags.n;
	return 0;
}
uint8_t Cpu6502::ARR() {
	uint8_t mem = nes->bus.read(target_addr);
	acc &= mem;

	uint8_t old_carry = flags.c;
	acc = (acc >> 1) | (old_carry << 7);

	set_nz(acc);

	flags.c = (acc >> 6) & 0x01;
	flags.v = ((acc >> 6) ^ (acc >> 5)) & 0x01;

	return 0;
}
uint8_t Cpu6502::AXS() {
	uint8_t mem = nes->bus.read(target_addr);
	uint8_t combined = acc & x_ind;
	uint16_t result = (uint16_t)combined - (uint16_t)mem;

	flags.c = (combined >= mem);
	x_ind = (uint8_t)(result & 0xFF);
	set_nz(x_ind);

	return 0;
}
uint8_t Cpu6502::LAX() {
	acc = nes->bus.read(target_addr);
	x_ind = acc;
	set_nz(x_ind);
	return 0;
}
uint8_t Cpu6502::SAX() {
	uint8_t result = acc & x_ind;
	nes->bus.write(target_addr, result);
	return 0;
}

// RMW Instructions

// DCP = DEC + CMP
uint8_t Cpu6502::DCP() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	// DEC
	mem--;

	// Write back to memory
	nes->bus.write(target_addr, mem);

	// CMP
	uint8_t cmp = acc - mem;
	flags.c = (acc >= mem);
	flags.z = (acc == mem);
	flags.n = (cmp & 0x80) ? 1 : 0;

	return 0;
}

// ISC = INC + SBC
uint8_t Cpu6502::ISC() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	// INC
	mem++;
	
	// Write back to memory
	nes->bus.write(target_addr, mem);

	// SBC
	uint16_t flipped_mem = (uint16_t)mem ^ 0x00FF;
	uint16_t result = acc + flipped_mem + flags.c;
	flags.c = (result > 0xFF) ? 1 : 0;
	flags.v = ((result ^ (uint16_t)acc) & (result ^ flipped_mem) & 0x80) ? 1 : 0;
	set_nz(result);
	acc = (uint8_t)result;
	return 0;
}

// RLA = ROL + AND
uint8_t Cpu6502::RLA() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	uint8_t old_carry = flags.c;
	flags.c = (mem >> 7) & 0x01;

	mem = (mem << 1) | old_carry;

	// Write back to memory
	nes->bus.write(target_addr, mem);

	// AND
	acc = acc & mem;
	set_nz(acc);
	return 0;
}

// RRA = ROR + ADC
uint8_t Cpu6502::RRA() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	// ROR
	uint8_t old_carry = flags.c;
	flags.c = mem & 0x01;
	mem = (mem >> 1) | (old_carry << 7);

	// Write back to memory
	nes->bus.write(target_addr, mem);
	
	// ADC
	uint16_t result = (uint16_t)acc + (uint16_t)mem + (uint16_t)flags.c;
	flags.c = (result > 0xFF) ? 1 : 0;
	flags.v = ((result ^ (uint16_t)acc) & (result ^ (uint16_t)mem) & 0x80) ? 1 : 0;
	set_nz(result);
	acc = result;
	return 0;
}

// SLO = ASL + ORA
uint8_t Cpu6502::SLO() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	// ASL
	flags.c = (mem >> 7) & 0x01;
	mem <<= 1;

	// Write back to memory
	nes->bus.write(target_addr, mem);

	// ORA
	acc = acc | mem;

	set_nz(acc);

	return 0;
}

// SRE = LSR + EOR
uint8_t Cpu6502::SRE() {
	uint8_t mem = nes->bus.read(target_addr);
	nes->bus.write(target_addr, mem);

	// LSR
	flags.c = mem & 0x01; // Bit 0
	mem >>= 1;
	flags.z = (mem == 0) ? 1 : 0;
	flags.n = 0;

	// Write back to memory
	nes->bus.write(target_addr, mem);

	// EOR
	acc = acc ^ mem;
	set_nz(acc);
	return 0;
}


// Unimplemented addressing modes
uint8_t Cpu6502::SHX() {
	uint8_t hi = (target_addr - y_ind) >> 8;
	uint8_t val = x_ind & (hi + 1);
	nes->bus.write(target_addr, val);
	return 0;
}
uint8_t Cpu6502::SHY() {
	uint8_t hi = (target_addr - x_ind) >> 8;
	uint8_t val = y_ind & (hi + 1);
	nes->bus.write(target_addr, val);
	return 0;
}


// ----- Addressing Modes -----

// Implicit
uint8_t Cpu6502::IMP() {
	nes->bus.read(PC);
	return 0;
}

// Accumulator
uint8_t Cpu6502::ACC() {
	nes->bus.read(PC);
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
	uint8_t addr = nes->bus.read(PC);
	PC++;
	target_addr = (uint16_t)addr;
	return 0;
}

// Zero Page Indexed (x)
uint8_t Cpu6502::ZPX() {
	uint8_t base_addr = nes->bus.read(PC);
	PC++;
	nes->bus.read(base_addr);
	target_addr = (uint16_t)(uint8_t)(base_addr + x_ind);
	return 0;
}

// Zero Page Indexed (y)
uint8_t Cpu6502::ZPY() {
	uint8_t base_addr = nes->bus.read(PC);
	nes->bus.read(base_addr);
	PC++;
	target_addr = (uint16_t)(uint8_t)(base_addr + y_ind);
	return 0;
}

// Absolute
uint8_t Cpu6502::AB0() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = nes->bus.read(PC);
	PC++;
	uint8_t hi = nes->bus.read(PC);
	PC++;

	target_addr = (hi << 8) | lo;
	return 0;
}

// Absolute Indexed (x)
uint8_t Cpu6502::ABX() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = nes->bus.read(PC);
	PC++;
	uint8_t hi = nes->bus.read(PC);
	PC++;

	uint16_t addr = (hi << 8) | lo;

	target_addr = addr + x_ind;

	bool is_rmw = (opcode == 0x1E || opcode == 0x3E || opcode == 0x5E ||
		opcode == 0x7E || opcode == 0xDE || opcode == 0xFE);
	bool page_crossed = (addr & 0xFF00) != (target_addr & 0xFF00);
	bool is_write = (il[opcode].name == "STA");

	// Check if a page boundary is crossed
	if (page_crossed || is_write || is_rmw) {
		uint16_t dummy_addr = (hi << 8) | (uint8_t)(lo + x_ind);
		nes->bus.read(dummy_addr);

		if (is_write || is_rmw) return 0;
		return 1; // Oops cycle
	}

	return 0;
}

// Absolute Indexed (y)
uint8_t Cpu6502::ABY() {
	// Fetches the value from a 16-bit address anywhere in memory.
	uint8_t lo = nes->bus.read(PC++);
	uint8_t hi = nes->bus.read(PC++);

	uint16_t addr = (hi << 8) | lo;
	target_addr = addr + y_ind;

	bool is_write = (il[opcode].name == "STA");

	if ((addr & 0xFF00) != (target_addr & 0xFF00) || is_write) {
		uint16_t dummy_addr = (hi << 8) | (uint8_t)(lo + y_ind);
		nes->bus.read(dummy_addr);

		if (is_write) return 0;

		return 1; // Oops cycle
	}

	return 0;
}

// Relative
uint8_t Cpu6502::REL() {
	uint8_t u_offset = nes->bus.read(PC);
	int8_t offset = (int8_t)(u_offset);
	PC++;

	target_addr = PC + offset;

	return 0;
}

// Indirect
uint8_t Cpu6502::IND() {
	uint8_t lo = nes->bus.read(PC);
	PC++;
	uint8_t hi = nes->bus.read(PC);
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

	uint16_t target_lo = nes->bus.read(ptr);
	uint16_t target_hi = nes->bus.read(ptr_hi_addr);


	target_addr = (target_hi << 8) | target_lo;

	return 0;
}

// Indirect Indexed (x)
uint8_t Cpu6502::IZX() {
	uint8_t base = nes->bus.read(PC++);
	nes->bus.read(base);
	uint8_t lo_addr = (uint8_t)(base + x_ind);
	uint8_t hi_addr = (uint8_t)(lo_addr + 1);

	uint8_t lo = nes->bus.read((uint16_t)lo_addr);
	uint8_t hi = nes->bus.read((uint16_t)hi_addr);

	target_addr = (hi << 8) | lo;

	return 0;
}

// Indirect Indexed (y)
uint8_t Cpu6502::IZY() {
	uint8_t ial = nes->bus.read(PC++);
	uint8_t lo = nes->bus.read((uint16_t)ial);
	uint8_t hi = nes->bus.read((uint16_t)(uint8_t)(ial + 1));

	uint16_t base_addr = (hi << 8) | lo;
	target_addr = base_addr + y_ind;

	bool is_write = (il[opcode].name == "STA");
	bool page_crossed = (base_addr & 0xFF00) != (target_addr & 0xFF00);

	if (page_crossed || is_write) {
		uint16_t dummy_addr = (hi << 8) | (uint8_t)(lo + y_ind);
		nes->bus.read(dummy_addr);

		if (is_write) return 0;
		return 1; // Oops cycle
	}

	return 0;
}