// Nes.h

#pragma once

#include "PPU.h"
#include "Bus.h"
#include "Cpu6502.h"

class Nes {
	public:
		enum Region {
			PAL,
			NTSC
		};

		Region region;
		float ppu_ratio;

		void set_region(Region region);

		float ppu_accumulator;

		Nes();

		void init();
		void tick();

		PPU ppu;
		Cpu6502 cpu6502;
		Bus bus;

	private:
		uint8_t cycles_remaining = 0;
		uint8_t clock = 0;
};