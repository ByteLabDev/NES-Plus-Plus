// src/APU.cpp
// https://www.nesdev.org/wiki/APU

#include "APU.h"
#include "Nes.h"
#include "Common.h"
#include "Cpu6502.h"

APU::APU(Nes* nesPtr) {
	nes = nesPtr;
	current_seq = nes->region == Common::Region::PAL ? PAL_Seq : NTSC_Seq;
}

bool APU::write(uint16_t addr, uint8_t data) {
	switch (addr) {
		case 0x4000:	// Pulse1 - Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
			pulse1.ctrl.reg = data;
			pulse1.envelope.loop = pulse1.ctrl.halt; // Same bit
			pulse1.envelope.constant_volume = pulse1.ctrl.constant;
			pulse1.envelope.reload_value = pulse1.ctrl.volume;
			break;
		case 0x4001:	// Pulse1 - Sweep unit: enabled (E), period (P), negate (N), shift (S) 
			pulse1.sweep.enabled = (data & 0x80);
			pulse1.sweep.period = (data & 0x70) >> 4;
			pulse1.sweep.negate = (data & 0x08);
			pulse1.sweep.shift = (data & 0x07);
			pulse1.sweep.reload = true;
			break;
		case 0x4002:	// Pulse1 - Timer low (T) 
			pulse1.timer_reload = (pulse1.timer_reload & 0x0700) | data;
			break;
		case 0x4003:	// Pulse1 - Length counter load (L), timer high (T) 
			pulse1.timer_reload = (pulse1.timer_reload & 0x00FF) | ((data & 0x07) << 8);
			pulse1.length_counter = length_table[(data & 0xF8) >> 3];
			pulse1.duty_phase = 0;
			pulse1.envelope.start = true;
			break;
		case 0x4004:	// Pulse2 - Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
			pulse2.ctrl.reg = data;
			pulse2.envelope.loop = pulse2.ctrl.halt; // Same bit
			pulse2.envelope.constant_volume = pulse2.ctrl.constant;
			pulse2.envelope.reload_value = pulse2.ctrl.volume;
			break;
		case 0x4005:	// Pulse2 - Sweep unit: enabled (E), period (P), negate (N), shift (S) 
			pulse2.sweep.enabled = (data & 0x80);
			pulse2.sweep.period = (data & 0x70) >> 4;
			pulse2.sweep.negate = (data & 0x08);
			pulse2.sweep.shift = (data & 0x07);
			pulse2.sweep.reload = true;
			break;
		case 0x4006:	// Pulse2 - Timer low (T) 
			pulse2.timer_reload = (pulse2.timer_reload & 0x0700) | data;
			break;
		case 0x4007:	// Pulse2 - Length counter load (L), timer high (T) 
			pulse2.timer_reload = (pulse2.timer_reload & 0x00FF) | ((data & 0x07) << 8);
			pulse2.length_counter = length_table[(data & 0xF8) >> 3];
			pulse2.duty_phase = 0;
			pulse2.envelope.start = true;
			break;
		case 0x4008:	// Triangle - Length counter halt / linear counter control (C), linear counter load (R)
			triangle.halt_flag = (data & 0x80);
			triangle.linear_reload_value = (data & 0x7F);
			break;
		case 0x400A:	// Triangle - Timer low (T) 
			triangle.timer_reload = (triangle.timer_reload & 0x0700) | data;
			break;
		case 0x400B:	// Triangle - Length counter load (L), timer high (T), set linear counter reload flag 
			triangle.timer_reload = (triangle.timer_reload & 0x00FF) | ((data & 0x07) << 8);
			triangle.length_counter = length_table[(data & 0xF8) >> 3];
			triangle.linear_reload_flag = true;
			break;
		case 0x400C:	// Noise - Envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
			noise.ctrl.reg = data;
			noise.envelope.loop = noise.ctrl.halt;
			noise.envelope.constant_volume = noise.ctrl.constant;
			noise.envelope.reload_value = noise.ctrl.volume;
			break;
		case 0x400E:	// Noise - Noise mode (M), noise period (P) 
			noise.mode = (data & 0x80);
			noise.timer_reload = (nes->region == Common::Region::PAL) ? noise.pal_rates[data & 0x0F] : noise.ntsc_rates[data & 0x0F];
			break;
		case 0x400F:	// Noise - Length counter load (L) 
			noise.length_counter = length_table[(data & 0xF8) >> 3];
			noise.envelope.start = true;
			break;
		case 0x4010:	// DMC - IRQ enable (I), loop (L), frequency (R)
			dmc.irq_enabled = (data & 0x80);
			dmc.loop = (data & 0x40);

			// Choose table based on the current region
			if (nes->region == Common::Region::PAL) {
				dmc.timer_reload = dmc.pal_rates[data & 0x0F];
			}
			else {
				dmc.timer_reload = dmc.ntsc_rates[data & 0x0F];
			}
			break;
		case 0x4011:	// DMC - Load counter (D) 
			dmc.output_level = (data & 0x7F);
			break;
		case 0x4012:	// DMC - Sample address (A) 
			dmc.sample_start_addr = 0xC000 + (uint16_t(data) << 6);
			break;
		case 0x4013:	// DMC - Sample length (L) 
			dmc.sample_length = (uint16_t(data) << 4) + 1;
			break;
		case 0x4015:	// Status
			pulse1.enabled = data & 0x01;
			pulse2.enabled = data & 0x02;
			triangle.enabled = data & 0x04;
			noise.enabled = data & 0x08;
			dmc.enabled = data & 0x10;

			if (!pulse1.enabled)   pulse1.length_counter = 0;
			if (!pulse2.enabled)   pulse2.length_counter = 0;
			if (!triangle.enabled) triangle.length_counter = 0;
			if (!noise.enabled) noise.length_counter = 0;

			if (dmc.enabled) {
				if (dmc.bytes_remaining == 0) {
					dmc.current_addr = dmc.sample_start_addr;
					dmc.bytes_remaining = dmc.sample_length;
					dmc_fetch_sample(); // Trigger initial fetch to fill the buffer
				}
			}
			else {
				dmc.bytes_remaining = 0;
			}
			break;
		case 0x4017:	// Frame Counter
			frame_value = 0;
			frame_irq_disable = (data & 0x40);
			is_5_step_mode = (data & 0x80);

			frame_irq_pending = false;

			if (is_5_step_mode) {
				// In 5-step mode, clock everything immediately
				triangle.clock_linear_counter();
				pulse1.clock_length_counter();
				pulse2.clock_length_counter();
				triangle.clock_length_counter();
				pulse1.clock_sweep(1);
				pulse2.clock_sweep(2);
			}
			
			break;
	}
	return true;
}

bool APU::read(uint16_t addr, uint8_t& data) {
	if (addr == 0x4015) {
		data = 0;
		if (pulse1.length_counter > 0)  data |= 0x01;
		if (pulse2.length_counter > 0)  data |= 0x02;
		if (triangle.length_counter > 0) data |= 0x04;
		if (noise.length_counter > 0) data |= 0x08;
		if (dmc.bytes_remaining > 0)    data |= 0x10; // Tell the game DMC is busy

		if (frame_irq_pending) {
			data |= 0x40;
		}

		frame_irq_pending = false;
		return true;
	}
	return false;
}

void APU::step() {
	// Clock triangle and DMC every CPU cycle
	triangle.clock_timer();
	dmc.clock_timer(this);
	noise.clock_timer();

	// Clock pulse timers every other CPU cycle
	if (total_cycles % 2 == 0) {
		pulse1.clock_timer();
		pulse2.clock_timer();
		clock_frame_counter();
	}

	// The Frame Counter manages high-level timing (approx 240Hz)

	total_cycles++;

	if (total_cycles % 400 == 0) {
		update_debug_history();
	}

	// Always signal the CPU if the flag is up
	nes->cpu6502.set_irq_line(frame_irq_pending);
}

void APU::clock_frame_counter() {
	frame_value++;

	if (frame_value == current_seq.steps[0] || frame_value == current_seq.steps[1] ||
		frame_value == current_seq.steps[2] || frame_value == current_seq.steps[3]) {

		triangle.clock_linear_counter();

		pulse1.envelope.clock();
		pulse2.envelope.clock();

		noise.envelope.clock();

		// Step 1 and 3 are the "Half-Frame" signals
		if (frame_value == current_seq.steps[1] || frame_value == current_seq.steps[3]) {
			pulse1.clock_length_counter();
			pulse2.clock_length_counter();
			pulse1.clock_sweep(1);
			pulse2.clock_sweep(2);

			triangle.clock_length_counter();
			noise.clock_length_counter();
		}
	}

	if (!is_5_step_mode) {
		// In 4-step mode, the Frame IRQ flag is set during these cycles
		// $4017 bit 6 (frame_irq_disable) must be 0 for this to happen
		if (frame_value == 14914 || frame_value == 14915 || frame_value == 0) {
			if (!frame_irq_disable) {
				frame_irq_pending = true;
			}
		}
	}

	if (frame_value >= current_seq.max_value) {
		frame_value = 0;
	}
}

void APU::dmc_fetch_sample() {
	// Only fetch if we have bytes left and the buffer is empty
	if (dmc.bytes_remaining > 0 && !dmc_sample_buffer_full) {
		// Read from the bus
		dmc_sample_buffer = nes->bus.read(dmc.current_addr);
		dmc_sample_buffer_full = true;

		// Increment address with wrap-around logic
		dmc.current_addr++;
		if (dmc.current_addr == 0x0000) { // Wrap from $FFFF to $8000
			dmc.current_addr = 0x8000;
		}

		dmc.bytes_remaining--;
		if (dmc.bytes_remaining == 0) {
			if (dmc.loop) {
				dmc.current_addr = dmc.sample_start_addr;
				dmc.bytes_remaining = dmc.sample_length;
			}
			else if (dmc.irq_enabled) {
				// nes->cpu6502.trigger_irq(); // Trigger IRQ if implemented
			}
		}
	}
}

void APU::DmcChannel::clock_timer(APU* apu_ptr) {
	if (timer_value == 0) {
		timer_value = timer_reload;

		// 1. Only update the DAC if we aren't silent
		if (!silent) {
			if (shift_register & 0x01) {
				if (output_level <= 125) output_level += 2;
			}
			else {
				if (output_level >= 2) output_level -= 2;
			}
		}

		// 2. Always shift and decrement, even if silent
		shift_register >>= 1;
		bits_remaining--;

		// 3. End of an output cycle (8 bits)
		if (bits_remaining == 0) {
			bits_remaining = 8;

			if (apu_ptr->dmc_sample_buffer_full) {
				// Buffer -> Shifter
				shift_register = apu_ptr->dmc_sample_buffer;
				apu_ptr->dmc_sample_buffer_full = false;
				silent = false;

				// Immediately refill the buffer from memory
				apu_ptr->dmc_fetch_sample();
			}
			else {
				// No more data in buffer? Silence the output
				silent = true;
			}
		}
	}
	else {
		timer_value--;
	}
}
float APU::get_output() {
	float p1 = debug.p1_enabled ? (float)pulse1.get_sample() : 0.0f;
	float p2 = debug.p2_enabled ? (float)pulse2.get_sample() : 0.0f;
	float tri = debug.tri_enabled ? (float)triangle.get_sample() : 0.0f;
	float dmc_val = debug.dmc_enabled ? (float)dmc.get_sample() : 0.0f;
	float noise_val = debug.noise_enabled ? (float)noise.get_sample() : 0.0f;

	// 1. Pulse Mixer
	float pulse_out = 0.0f;
	if (p1 + p2 != 0) {
		pulse_out = 95.88f / (8128.0f / (p1 + p2) + 100.0f);
	}

	// 2. TND Mixer (Triangle, Noise, DMC)
	float tnd_out = 0.0f;
	float tnd_denominator = (tri / 8227.0f) + (noise_val / 12241.0f) + (dmc_val / 22638.0f);
	if (tnd_denominator != 0.0f) {
		tnd_out = 159.79f / ((1.0f / tnd_denominator) + 100.0f);
	}

	float current_sample = pulse_out + tnd_out;

	// 3. Filter to prevent popping
	float filtered_out = 0.996f * (high_pass_prev_out + current_sample - high_pass_prev_sample);

	high_pass_prev_sample = current_sample;
	high_pass_prev_out = filtered_out;

	return filtered_out;
}

void APU::update_debug_history() {
	debug.p1_history[debug.write_idx] = (float)pulse1.get_sample();
	debug.p2_history[debug.write_idx] = (float)pulse2.get_sample();
	debug.tri_history[debug.write_idx] = (float)triangle.get_sample();
	debug.dmc_history[debug.write_idx] = (float)dmc.get_sample();
	debug.noise_history[debug.write_idx] = (float)noise.get_sample();
	debug.mixed_history[debug.write_idx] = get_output();

	debug.write_idx = (debug.write_idx + 1) % 100;
}