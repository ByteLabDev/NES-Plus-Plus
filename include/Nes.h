// Nes.h

#pragma once

#include "PPU.h"
#include "Bus.h"
#include "Display.h"
#include "Cpu6502.h"
#include "Cartridge.h"
#include "Controller.h"

class Nes {
	public:
		enum Region {
			PAL,
			NTSC
		};

		Region region;
		float ppu_ratio;

		void set_region(Region region);
		void insert_cartridge(Cartridge* cartridgePtr);

		Nes();

		void init();
		void tick();

		PPU ppu;
		Cpu6502 cpu6502;
		Bus bus;
		Display display;
		Cartridge* cartridge;
		Controller controller;

	private:
		uint8_t clock = 0;
		float ppu_accumulator = 0;
};