// include/APU.h

#pragma once
#include <cstdint>
#include "Common.h"

class Nes;

class APU {
	public:
		APU(Nes* nesPtr);
		bool read(uint16_t addr, uint8_t& data);
		bool write(uint16_t addr, uint8_t data);
		void step();
		float get_output();
		void set_region(Common::Region region) {
			current_seq = (region == Common::Region::PAL) ? PAL_Seq : NTSC_Seq;
		}

		struct DebugData {
			float p1_history[100];
			float p2_history[100];
			float tri_history[100];
			float dmc_history[100];
			float noise_history[100];
			float mixed_history[100];
			int write_idx = 0;

			bool p1_enabled = true;
			bool p2_enabled = true;
			bool dmc_enabled = true;
			bool tri_enabled = true;
			bool noise_enabled = true;
		} debug;
	private:
		Nes* nes;

		const uint8_t length_table[32] = {
			10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
		};

		struct FrameSequence {
			uint32_t steps[4];
			uint32_t max_value;
		};

		const FrameSequence NTSC_Seq = { {3728, 7456, 11185, 14914}, 14915 };
		const FrameSequence PAL_Seq = { {4156, 8313, 12469, 16627}, 16628 };

		FrameSequence current_seq;

		union PulseControl {
			struct {
				uint8_t volume		: 4;	// Volume/envelope divider period
				uint8_t constant	: 1;	// Constant volume/envelope flag
				uint8_t halt		: 1;	// Length counter halt
				uint8_t duty		: 2;	// Duty cycle
			};
			uint8_t reg;
		};

		struct Envelope {
			bool start = false;
			bool loop = false;
			bool constant_volume = false;
			uint8_t volume_out = 0;
			uint8_t decay_count = 0;
			uint8_t divider = 0;
			uint8_t reload_value = 0; // The 'V' in the registers

			void clock() {
				if (!start) {
					if (divider == 0) {
						divider = reload_value; // The 'V' value from $4000
						if (decay_count > 0) {
							decay_count--;
						}
						else if (loop) {
							decay_count = 15;
						}
					}
					else {
						divider--;
					}
				}
				else {
					start = false;
					decay_count = 15;
					divider = reload_value;
				}
				volume_out = constant_volume ? reload_value : decay_count;
			}
		};

		struct PulseChannel {
			PulseControl ctrl;
			uint16_t timer_reload;
			uint8_t length_counter;
			uint16_t timer_value;
			uint8_t  duty_phase;
			bool enabled = false;

			struct Sweep {
				bool enabled = false;
				uint8_t period = 0;
				uint8_t divider = 0; // Current countdown
				bool negate = false;
				uint8_t shift = 0;
				bool reload = false;
			} sweep;

			Envelope envelope;

			void clock_sweep(int pulse_id) {
				uint16_t delta = timer_reload >> sweep.shift;
				uint16_t target_period = timer_reload;

				if (sweep.negate) {
					// NEGATE TRUE: Period decreases -> Pitch slides UP
					if (pulse_id == 1) target_period = timer_reload - delta - 1;
					else target_period = timer_reload - delta;
				}
				else {
					// NEGATE FALSE: Period increases -> Pitch slides DOWN
					target_period = timer_reload + delta;
				}

				// --- Muting Logic ---
				// The sweep unit mutes the channel if the target period > 0x7FF
				bool mute = (timer_reload < 8) || (target_period > 0x7FF);

				if (sweep.enabled && sweep.shift > 0 && !mute && sweep.divider == 0) {
					timer_reload = target_period;
				}

				// --- Divider Logic ---
				if (sweep.divider == 0 || sweep.reload) {
					sweep.divider = sweep.period;
					sweep.reload = false;
				}
				else {
					sweep.divider--;
				}
			}

			void clock_timer() {
				if (timer_value == 0) {
					timer_value = timer_reload;
					duty_phase = (duty_phase + 1) % 8;
				}
				else {
					timer_value--;
				}
			}

			void clock_length_counter() {
				if (length_counter > 0 && !ctrl.halt) {
					length_counter--;
				}
			}

			uint8_t get_sample() {
				if (length_counter == 0 || timer_reload < 8) return 0;

				uint16_t delta = timer_reload >> sweep.shift;
				// If not negating, check if we've slid off the bottom of the frequency range
				if (!sweep.negate && (timer_reload + delta) > 0x07FF) return 0;

				static const uint8_t duty_table[4][8] = {
					{0, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0},
					{0, 1, 1, 1, 1, 0, 0, 0}, {1, 0, 0, 1, 1, 1, 1, 1}
				};
				return duty_table[ctrl.duty][duty_phase] ? envelope.volume_out : 0;
			}
		};

		struct TriangleChannel {
			uint8_t linear_counter = 0;
			uint8_t linear_reload_value = 0;
			bool linear_reload_flag = false;
			bool halt_flag = false;

			uint16_t timer_reload = 0;
			uint16_t timer_value = 0;
			uint8_t length_counter = 0;
			uint8_t step_index = 0;					// The triangle wave has 32 steps that output a 4-bit value.
			bool enabled = false;

			const uint8_t tri_wave_table[32] = {
				15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
				0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
			};

			void clock_timer() {
				if (timer_value == 0) {
					timer_value = timer_reload;
					if (length_counter > 0 && linear_counter > 0 && timer_reload >= 2) {
						step_index = (step_index + 1) % 32;
					}
				}
				else {
					timer_value--;
				}
			}

			void clock_linear_counter() {
				if (linear_reload_flag) {
					linear_counter = linear_reload_value;
				}
				else if (linear_counter > 0) {
					linear_counter--;
				}

				if (!halt_flag) {
					linear_reload_flag = false;
				}
			}

			void clock_length_counter() {
				if (length_counter > 0 && !halt_flag) {
					length_counter--;
				}
			}

			uint8_t get_sample() const {
				if (length_counter == 0 || linear_counter == 0 || timer_reload < 2) {
					return 7;
				}
				return tri_wave_table[step_index];
			}
		};

		struct DmcChannel {
			uint16_t timer_reload = 0;   // Derived from 'freq' (R)
			uint8_t  output_level = 0;   // The 7-bit DAC value (D) - start at 0
			uint16_t sample_start_addr = 0; // Derived from (A): 0xC000 + A*64
			uint16_t sample_length = 0;  // Derived from (L): L*16 + 1

			bool irq_enabled = false;    // (I)
			bool loop = false;           // (L)
			bool enabled = false;

			uint16_t timer_value = 0;
			uint16_t current_addr = 0;   // Pointer moving through memory
			uint16_t bytes_remaining = 0;// Countdown for the sample length

			uint8_t  shift_register = 0; // Holds the 8 bits of the current byte
			uint8_t  bits_remaining = 8; // How many bits left in shift_register (0-8)
			bool     silent = true;      // If the sample buffer is empty

			const uint16_t ntsc_rates[16] = {
				428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
			};

			const uint16_t pal_rates[16] = {
				398, 354, 316, 298, 266, 236, 210, 198, 176, 148, 132, 118, 98, 78, 66, 50
			};

			uint8_t get_sample() const {
				return output_level;
			}

			void clock_timer(APU* apu_ptr);
		};

		struct NoiseChannel {
			PulseControl ctrl;
			Envelope envelope;

			uint16_t timer_reload = 0;
			uint16_t timer_value = 0;
			uint8_t  length_counter = 0;
			bool     enabled = false;
			bool     mode = false;
			uint16_t shift_register = 1; // Must be non-zero at reset

			const uint16_t ntsc_rates[16] = {
				4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
			};

			const uint16_t pal_rates[16] = {
				4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778
			};

			void clock_timer() {
				if (timer_value == 0) {
					timer_value = timer_reload;

					// LFSR Logic
					// Feedback bit is bit 0 XOR (bit 1 if mode else bit 6)
					uint16_t feedback = (shift_register & 0x01) ^ ((mode ? (shift_register >> 6) : (shift_register >> 1)) & 0x01);
					shift_register >>= 1;
					shift_register |= (feedback << 14);
				}
				else {
					timer_value--;
				}
			}

			void clock_length_counter() {
				if (length_counter > 0 && !ctrl.halt) {
					length_counter--;
				}
			}

			uint8_t get_sample() {
				if (length_counter == 0 || (shift_register & 0x01)) return 0;
				return envelope.volume_out;
			}
		};

		PulseChannel pulse1;
		PulseChannel pulse2;
		TriangleChannel triangle;
		DmcChannel dmc;
		NoiseChannel noise;

		uint64_t total_cycles;
		uint32_t frame_value = 0;

		uint8_t dmc_sample_buffer = 0;
		bool dmc_sample_buffer_full = false;

		float high_pass_prev_sample = 0.0f;
		float high_pass_prev_out = 0.0f;

		void clock_frame_counter();
		void dmc_fetch_sample();
		void update_debug_history();
};