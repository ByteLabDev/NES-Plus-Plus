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
	private:
		Nes* nes;

		const uint8_t length_table[32] = {
			10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
		};

		const uint8_t tri_table[32] = {
			15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
		};

		struct FrameSequence {
			uint32_t steps[4];
			uint32_t max_value;
		};

		const FrameSequence NTSC_Seq = { {3728, 7456, 11185, 14914}, 14915 };
		const FrameSequence PAL_Seq = { {4156, 8313, 12469, 16627}, 16628 };

		FrameSequence current_seq = PAL_Seq;

		union PulseControl {
			struct {
				uint8_t volume		: 4;	// Volume/envelope divider period
				uint8_t constant	: 1;	// Constant volume/envelope flag
				uint8_t halt		: 1;	// Length counter halt
				uint8_t duty		: 2;	// Duty cycle
			};
			uint8_t reg;
		};

		struct PulseChannel {
			PulseControl ctrl;
			uint16_t timer_reload;
			uint8_t length_counter;
			uint16_t timer_value;
			uint8_t  duty_phase;
			bool enabled = false;

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

				static const uint8_t duty_table[4][8] = {
					{0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
					{0, 1, 1, 0, 0, 0, 0, 0}, // 25%
					{0, 1, 1, 1, 1, 0, 0, 0}, // 50%
					{1, 0, 0, 1, 1, 1, 1, 1}  // 25% negated
				};

				return duty_table[ctrl.duty][duty_phase] ? ctrl.volume : 0;
			}
		};

		PulseChannel pulse1;
		PulseChannel pulse2;

		uint8_t total_cycles;
		uint32_t frame_value = 0;

		void clock_frame_counter();
};