// include/Cartridge.h
// https://www.nesdev.org/wiki/INES

#pragma once
#include <cstdint>
#include <vector>
#include <fstream>
#include "Mapper.h"

class Nes;

struct iNES_Header {
	char	name[4];		// Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file) 
	uint8_t prg_rom_banks;	// Size of PRG ROM in 16 KB units 
	uint8_t chr_rom_banks;	// Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM) 
	uint8_t flags6;			// Mapper, mirroring, battery, trainer 
	uint8_t flags7;			// Mapper, VS/Playchoice, NES 2.0
	uint8_t prg_ram_banks;	// PRG-RAM size (rarely used extension)
	uint8_t tv_1;			// TV system (rarely used extension)
	uint8_t tv_2;			// TV system, PRG-RAM presence (unofficial, rarely used extension) 
	uint8_t unused[5];		// Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)
};

class Cartridge {
	public:
		enum MirrorMode {
			HORIZONTAL,
			VERTICAL,
			FOUR_SCREEN
		};
		Cartridge(Nes* nesPtr);
		bool read(uint16_t addr, uint8_t& data);
		bool write(uint16_t addr, uint8_t data);
		bool load_rom(const std::string& filename);
		bool load_rom();
		MirrorMode get_mirror_mode();
		bool is_loaded = false;
	private:
		Nes* nes;
		std::string romPath;
		std::vector<uint8_t> prgRom;	// Program instructions
		std::vector<uint8_t> chrRom;	// Character data
		Mapper mapper;					// Cartridge board mapper (https://www.nesdev.org/wiki/Cartridge_board_reference)
		MirrorMode mirror;
};