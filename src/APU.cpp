// src/APU.cpp
// https://www.nesdev.org/wiki/APU

#include "APU.h"

APU::APU(Nes* nesPtr) {
	nes = nesPtr;
}

bool APU::write(uint16_t addr, uint8_t data) {
	switch (addr) {
		case 0x4000:
			pulse1.ctrl.reg = data;
			break;
		case 0x4001:
			// TODO: Sweep
			break;
		case 0x4002:
			pulse1.timer_reload = (pulse1.timer_reload & 0x0700) | data;
			break;
		case 0x4003:
			pulse1.timer_reload = (pulse1.timer_reload & 0x00FF) | ((data & 0x07) << 8);
			pulse1.length_counter = length_table[(data & 0xF8) >> 3];
			pulse1.duty_phase = 0;
			break;

		case 0x4004:
			pulse2.ctrl.reg = data;
			break;
		case 0x4005:
			// TODO: Sweep
			break;
		case 0x4006:
			pulse2.timer_reload = (pulse2.timer_reload & 0x0700) | data;
			break;
		case 0x4007:
			pulse2.timer_reload = (pulse2.timer_reload & 0x00FF) | ((data & 0x07) << 8);
			pulse2.length_counter = length_table[(data & 0xF8) >> 3];
			pulse2.duty_phase = 0;
			break;
		case 0x4015:	// Status
			pulse1.enabled = data & 0x01;
			pulse2.enabled = data & 0x02;

			if(!pulse1.enabled) pulse1.length_counter = 0;
			if(!pulse2.enabled) pulse2.length_counter = 0;
			break;
		case 0x4017:	// Frame Counter
			break;
	}
	return true;
}

bool APU::read(uint16_t addr, uint8_t& data) {
	return true;
}

void APU::step() {
	if (total_cycles % 2 == 0) {
		pulse1.clock_timer();
		pulse2.clock_timer();
	}

	// The Frame Counter manages high-level timing (approx 240Hz)
	clock_frame_counter();

	total_cycles++;
}

void APU::clock_frame_counter() {
	frame_value++;

	// Approx. every 3728 cycles (NTSC)
	if (frame_value == 3728 || frame_value == 7456 ||
		frame_value == 11185 || frame_value == 14914) {

		// Clock Envelopes (Every step)
		// pulse1.envelope.step(); 

		// Clock Length Counters (Steps 2 and 4)
		if (frame_value == 7456 || frame_value == 14914) {
			pulse1.clock_length_counter();
		}
	}

	if (frame_value >= 14915) {
		frame_value = 0;
	}
}

float APU::get_output() {
	float p1 = (float)pulse1.get_sample();
	float p2 = (float)pulse2.get_sample();

	// Standard NES Mixer Formula for Pulse Channels
	if (p1 + p2 == 0) return 0.0f;
	return 95.88f / (8128.0f / (p1 + p2) + 100.0f);
}