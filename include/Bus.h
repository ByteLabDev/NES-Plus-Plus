#pragma once
#include <cstdint>
#include <fstream>
#include <array>

class Cartridge;
class PPU;
class Nes;

class Bus {
	private:
		std::array<uint8_t, 2048> wram = {};		// Working Memory (2kb = 2^11b)
		Cartridge* cartridge;
		Nes* nes;
	public:
		Bus(Nes* nesPtr);
		void clear();
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t data);
		void insertCartridge(Cartridge* cartridgePtr);
};