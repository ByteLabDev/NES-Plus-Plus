// src/Nes.cpp

#include "Nes.h"
#include <iostream>
#include "Common.h"

using namespace Common;

Nes::Nes() : ppu(this), cpu6502(this), bus(this), display(this), controller(this), apu(this), audio(this) {
	set_region(Region::NTSC); // Temp: Hardcode region to NTSC
}

void Nes::set_region(Region region) {
    target_frame_rate = (region == Region::PAL) ? 50 : 60;
    cpu_clock_speed = region == PAL ? 1662607.0 : 1789773.0;
    samples_per_nes_clock = (double)sample_rate / (double)cpu_clock_speed;
}

void Nes::insert_cartridge(Cartridge* cartridgePtr) {
	cartridge = cartridgePtr;
	bus.insertCartridge(cartridgePtr);
}

void Nes::tick() {
    uint8_t cycles = cpu6502.step();

    // The APU must stay in sync with the CPU cycles
    for (uint8_t i = 0; i < cycles; i++) {
        apu.step();

        // Generate audio samples at the correct interval
        sample_time += samples_per_nes_clock;
        if (sample_time >= 1.0) {
            sample_time -= 1.0;
            audio_queue.push_back(apu.get_output());
        }
    }

    // PAL ratio is 3.2 (16:5), NTSC is 3:1 (15:5)
    // This is more accurate than checking every 3.2 cycles because of floating point errors
    uint8_t multiplier = (region == PAL) ? 16 : 15;
    uint8_t divisor = 5;

    // PPU still scales based on the total instruction cycles
    ppu_accumulator += cycles * multiplier;
    while (ppu_accumulator >= divisor) {
        ppu.step();
        ppu_accumulator -= divisor;
    }

    // Push to SDL once we have enough for a frame
    int max_audio_queue_size = sample_rate / target_frame_rate;
    if (audio_queue.size() >= max_audio_queue_size) {
        audio.push_samples(audio_queue.data(), (int)audio_queue.size());
        audio_queue.clear();
    }
}