// https://www.nesdev.org/wiki/CPU_memory_map

#include "Bus.h"

Bus::Bus(PPU* ppuPtr) {
	ppu = ppuPtr;
}

uint8_t Bus::read(uint16_t addr) {
	// Working RAM:				$0000 - $1FFF (0-8191)
	// Non-mirror range:		$0000–$07FF (0-2047)
	if (addr <= 0x1FFF) {
		uint16_t w_addr = addr % 2048;	// Wrap to avoid mirrors.
		return wram[w_addr];
	}

	// PPU Registers:			$2000 – $3FFF (8192 - 16383)
	// Non-mirror range:		$2000 – $2007 (8192 - 8200)
	if (addr >= 0x2000 && addr <= 0x3FFF) {
		uint16_t w_addr = (addr - 0x2000) % 8; // Wrap every 8 steps
		// TODO: Return PPU Registers
		return 0;
	}

	// APU, I/O Registers:		$4000 – $4017 (16384 - 16407)
	if (addr >= 0x4000 && addr <= 0x4017) {
		// TODO: Return APU and I/O registers
		return 0;
	}

	// Disabled APU, I/O Reg:	$4018–$401F (16408 - 16415)
	if (addr >= 0x4018 && addr <= 0x401F) {
		// APU and I/O functionality that is normally disabled. See CPU Test Mode. 
		return 0;
	}

	// Disabled APU, I/O Reg:	$4020–$FFFF (16416 - 65535)
	if (addr >= 0x4020 && addr <= 0xFFFF) {
		// APU and I/O functionality that is normally disabled. See CPU Test Mode.
		uint8_t data = 0;
		if (cartridge != nullptr && cartridge->read(addr, data)) {
			return data;
		}
	}

	return 0;
}

void Bus::write(uint16_t addr, uint8_t data) {
	// Working RAM:				$0000 - $1FFF (0-8191)
	// Non-mirror range:		$0000–$07FF (0-2047)
	if (addr <= 0x1FFF) {
		uint16_t w_addr = addr % 2048;	// Wrap to avoid mirrors.
		wram[w_addr] = data;
		return;
	}

	// PPU Registers:			($2000 – $3FFF) (8192 - 16383)
	// Non-mirror range:		($2000 – $2007) (8192 - 8200)
	if (addr >= 0x2000 && addr <= 0x3FFF) {
		uint16_t w_addr = (addr - 0x2000) % 8; // Wrap every 8 steps
		// TODO: Return PPU Registers
		return;
	}

	// APU, I/O Registers:		($4000 – $4017) (16384 - 16407)
	if (addr >= 0x4000 && addr <= 0x4017) {
		// TODO: Return APU and I/O registers
		return;
	}
}

void Bus::insertCartridge(Cartridge* cartridgePtr) {
	cartridge = cartridgePtr;
}