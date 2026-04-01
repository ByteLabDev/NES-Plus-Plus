#pragma once
#include <cstdint>
#include <fstream>
#include <array>
#include "Cartridge.h"

class Bus {
	private:
		std::array<uint8_t, 2048> wram;		// Working Memory (2kb = 2^11b)
		Cartridge* cartridge;
	public:
		Bus();
		void init();
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t data);
		void insertCartridge(Cartridge* cartridgePtr);
};