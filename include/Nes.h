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

		Nes();

		void init();
		void tick();

		PPU ppu;
		Cpu6502 cpu6502;
		Bus bus;

	private:
		uint8_t clock = 0;
		float ppu_accumulator = 0;
};