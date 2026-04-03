// Nes.cpp

#include "Nes.h"

Nes::Nes() : ppu(this), cpu6502(this), bus(this) {
	set_region(Region::PAL); // Temp: Hardcode region to PAL
}

void Nes::set_region(Region region) {
	ppu_ratio = (region == PAL) ? 3.2f : 3.0f;
}

void Nes::tick() {
	uint8_t cycles = cpu6502.step();

	ppu_accumulator += cycles * ppu_ratio;

	while (ppu_accumulator >= 1.0f) {
		ppu.step();
		ppu_accumulator -= 1;
	}

	clock++;
}