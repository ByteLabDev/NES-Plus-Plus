// include/Nes.h

#pragma once

#include "PPU.h"
#include "Bus.h"
#include "Display.h"
#include "Cpu6502.h"
#include "Cartridge.h"
#include "Controller.h"
#include "APU.h"
#include "Audio.h"
#include "Common.h"

class Nes {
	public:
		Common::Region region = Common::Region::NTSC;
		float ppu_ratio;
		int target_frame_rate;
		int cpu_clock_speed;
		const int sample_rate = 44100;

		void set_region(Common::Region region);
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
		APU apu;
		Audio audio;
	private:
		uint8_t clock = 0;
		float ppu_accumulator = 0;
		double sample_time = 0;
		double samples_per_nes_clock;
		std::vector<float> audio_queue;
};