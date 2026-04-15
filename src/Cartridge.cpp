#include "Cartridge.h"
#include <iostream>

Cartridge::Cartridge(const std::string& fileName) {
	romPath = fileName;
}

bool Cartridge::read(uint16_t addr, uint8_t& data) {
	if (addr < 2000 && !chrRom.empty()) {
		data = chrRom[addr];
		return true;
	}
	if (addr >= 0x8000) {
		uint16_t mask = prgRom.size() - 1;

		data = prgRom[addr & mask];
		return true;
	}
	return false;
}

bool Cartridge::write(uint16_t addr, uint8_t data) {
	if (addr >= 0x8000) {
		uint16_t mask = prgRom.size() - 1;

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

	// Bit 3 of flags6: 1 = Four-screen, 0 = Use Bit 0
	if (header.flags6 & 0x08) {
		mirror = FOUR_SCREEN;
	}
	else {
		// Bit 0 of flags6: 0 = Horizontal, 1 = Vertical
		mirror = (header.flags6 & 0x01) ? VERTICAL : HORIZONTAL;
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

Cartridge::MirrorMode Cartridge::get_mirror_mode() {
	return mirror;
}