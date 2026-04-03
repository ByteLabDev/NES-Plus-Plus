#include "Cartridge.h"
#include <iostream>

Cartridge::Cartridge(const std::string& fileName) {
	romPath = fileName;
}

bool Cartridge::read(uint16_t addr, uint8_t& data) {
	if (addr >= 0x8000) {
		// prgRom.size() is usually 16384 or 32768.
		// Subtracting 1 gives us a bitmask (0x3FFF or 0x7FFF).
		uint16_t mask = prgRom.size() - 1;

		// This maps 0x8000-0xFFFF directly into the 0x0000-size range
		data = prgRom[addr & mask];
		return true;
	}
	return false;
}

bool Cartridge::loadRom(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);

	if (!file.is_open()) {
		std::cerr << "Failed to open ROM file: " << filename << std::endl;
		return false;
	}

	iNES_Header header;

	file.read(reinterpret_cast<char*>(&header), sizeof(iNES_Header));

	if (std::string(header.name, 3) != "NES") {
		std::cerr << "File is not an NES ROM: " << filename << std::endl;
		return false;
	}

	// Skip trainer if bit 2 (0b100) of flags6 is set
	if (header.flags6 & 0b100) {
		file.seekg(512, std::ios::cur);
	}

	// Load PRG ROM
	prgRom.resize(header.prg_rom_banks * 16384);
	file.read(reinterpret_cast<char*>(prgRom.data()), prgRom.size());

	// Load CHR ROM
	if (header.chr_rom_banks > 0) {
		chrRom.resize(header.chr_rom_banks * 8192);
		file.read(reinterpret_cast<char*>(chrRom.data()), chrRom.size());
	}
	else {
		chrRom.resize(8192);
	}

	romPath = filename;
	return true;
}

bool Cartridge::loadRom() {
	return loadRom(romPath);
}