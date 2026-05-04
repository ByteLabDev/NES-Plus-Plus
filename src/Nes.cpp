// src/Nes.cpp

#include "Nes.h"
#include <iostream>
#include "Common.h"

using namespace Common;

Nes::Nes() : ppu(this), cpu6502(this), bus(this), display(this), controller(this), apu(this), audio(this) {
	set_region(Region::PAL); // Temp: Hardcode region to PAL
	samples_per_nes_clock = 44100.0 / (region == PAL ? 1662607.0 : 1789773.0);
}

void Nes::set_region(Region region) {
	ppu_ratio = (region == PAL) ? 3.2f : 3.0f;
}

void Nes::insert_cartridge(Cartridge* cartridgePtr) {
	cartridge = cartridgePtr;
	bus.insertCartridge(cartridgePtr);
}

void Nes::tick() {
	uint8_t cycles = cpu6502.step();
	apu.step();

	sample_time += samples_per_nes_clock;
	if (sample_time >= 1.0) {
		sample_time -= 1.0;
		audio_queue.push_back(apu.get_output());
	}

	// When the buffer gets reasonably full (e.g., every frame)
	if (audio_queue.size() >= 735) { // Roughly 1/60th of a second
		audio.push_samples(audio_queue.data(), audio_queue.size());
		audio_queue.clear();
	}

	ppu_accumulator += cycles * ppu_ratio;

	while (ppu_accumulator >= 1.0f) {
		ppu.step();
		ppu_accumulator -= 1;
	}

	clock++;
}