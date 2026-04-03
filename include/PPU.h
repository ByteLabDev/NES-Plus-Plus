#pragma once
#include <cstdint>

class PPU {
	public:
	private:
		// ------------------------------------
		// ---------- MMIO Registers ----------
		// ------------------------------------

		// ----- PPUCTRL - Miscellaneous settings ($2000 write) -----
		union PPUCTRL {
			struct {
				uint8_t nametable_x : 1;		// Base nametable address (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00) | X scroll position bit 8 (i.e. add 256 to X)
				uint8_t nametable_y : 1;		// Base nametable address (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00) | Y scroll position bit 8 (i.e. add 240 to Y)
				uint8_t increment_mode : 1;		// VRAM address increment per CPU read/write of PPUDATA (0: add 1, going across; 1: add 32, going down)
				uint8_t sp_pattern : 1;			// Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode)
				uint8_t bg_pattern : 1;			// Background pattern table address (0: $0000; 1: $1000)
				uint8_t sprite_size : 1;		// Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
				uint8_t slave_mode : 1;			// PPU master/slave select (0: read backdrop from EXT pins; 1: output color on EXT pins)
				uint8_t vblank_nmi_enable : 1;	// Vblank NMI enable (0: off, 1: on)
			};
			uint8_t reg;
		};


		// PPUMASK - Rendering settings ($2001 write)
		union PPUMASK {
			struct {
				uint8_t greyscale : 1;			// Greyscale (0: normal color, 1: greyscale)
				uint8_t render_bg_left : 1;		// 1: Show background in leftmost 8 pixels of screen, 0: Hide
				uint8_t render_sp_left : 1;		// 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
				uint8_t render_bg : 1;			// 1: Enable background rendering
				uint8_t render_sp : 1;			// 1: Enable sprite rendering
				uint8_t emph_r : 1;				// Emphasize red (green on PAL/Dendy)
				uint8_t emph_g : 1;				// Emphasize green (red on PAL/Dendy)
				uint8_t emph_b : 1				// Emphasize blue
			};
			uint8_t reg;
		};

		// ----- PPUSTATUS - Rendering events ($2002 read) -----
		union PPUSTATUS {
			struct {
				uint8_t unused : 5;
				uint8_t flag_sp_overflow : 1;	// Sprite overflow flag
				uint8_t flag_sp_0_hit : 1;		// Sprite 0 hit flag
				uint8_t flag_blank : 1;			// Vblank flag, cleared on read. Unreliable, see wiki: https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS_-_Rendering_events_($2002_read)
			};
			uint8_t reg;
		};

		// PPUADDR - VRAM address ($2006 write)
		PPUCTRL		ctrl;				// $2000
		PPUMASK		mask;				// $2001
		PPUSTATUS	status;				// $2002

		uint16_t	vram_addr;			// v register (Current VRAM address)
		uint16_t	tmp_vram_addr;		// t register (Temp VRAM address)
		uint8_t		fine_x;				// 3-bit fine X scroll
		bool		addr_latch;			// w register (Write toggle)

		uint8_t		vram_read_buffer;	// Delayed read buffer for $2007

		uint8_t		oam_addr;			// Sprite address
		uint8_t		oam_data[256];		// Sprite memory
};