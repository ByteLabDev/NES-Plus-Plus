#pragma once
#include <cstdint>

class Nes;

class PPU {
	public:
		PPU(Nes* nesPtr);
		uint16_t cycles;
		uint16_t scanline;
		void reset();
		void step();
		bool read(uint16_t addr, uint8_t& data);
		bool write(uint16_t addr, uint16_t data);
		uint8_t vram_read(uint16_t addr);
		void PPU::vram_write(uint16_t addr, uint8_t data);
		int c_count_temp = 0;
		uint32_t frame_buffer[256 * 240];	// 256x240 video output. 32 bits, 8 bits per color channel (RGBA)
		bool frame_ready = false;
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
				uint8_t emph_b : 1;				// Emphasize blue
			};
			uint8_t reg;
		};

		// ----- PPUSTATUS - Rendering events ($2002 read) -----
		union PPUSTATUS {
			struct {
				uint8_t unused : 5;
				uint8_t flag_sp_overflow : 1;	// Sprite overflow flag
				uint8_t flag_sp_0_hit : 1;		// Sprite 0 hit flag
				uint8_t flag_vblank : 1;			// Vblank flag, cleared on read. Unreliable, see wiki: https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS_-_Rendering_events_($2002_read)
			};
			uint8_t reg;
		};

		// PPUADDR - VRAM address ($2006 write)
		PPUCTRL		ctrl{ 0x0 };				// $2000
		PPUMASK		mask{ 0x0 };				// $2001
		PPUSTATUS	status{ 0x0 };				// $2002

		uint16_t	vram_addr;					// v register (Current VRAM address)
		uint16_t	tmp_vram_addr;				// t register (Temp VRAM address)
		uint8_t		fine_x;						// 3-bit fine X scroll
		bool		addr_latch;					// w register (Write toggle)

		uint8_t		vram_read_buffer;			// Delayed read buffer for $2007

		uint8_t		oam_addr;					// Sprite address
		uint8_t		oam_data[256];				// Sprite memory

		uint8_t		vram[2048];					// VRAM
		uint8_t		palette_ram[32];			// Palette memory

		uint16_t	bg_shifter_pattern_lo;		// BG Shifter Pattern Lo
		uint16_t	bg_shifter_pattern_hi;		// BG Shifter Pattern Hi
		uint16_t	bg_shifter_attrib_lo;		// BG Shifter Attributes Lo
		uint16_t	bg_shifter_attrib_hi;		// BG Shifter Attributes Hi

		Nes *nes;

		// ----- Helper Methods -----
		uint16_t get_mirror_index(uint16_t addr);
		void render_pixel();
		void update_shifters();
		void increment_scroll_x();

		const uint32_t system_palette[64] = {
			0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
			0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
			0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
			0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
			0xFFFEFF, 0x64B0FF, 0x9291FF, 0xC686FF, 0xED7CFF, 0xFE7E00, 0xFF8A47, 0xEFA317,
			0xC1BD00, 0x88D500, 0x5AE22E, 0x2FE377, 0x13D1CE, 0x4B4B4B, 0x000000, 0x000000,
			0xFFFEFF, 0xBADCFF, 0xCECDFF, 0xE4BEFF, 0xF5B7FF, 0xF9B8D8, 0xF9BD9E, 0xF2C78E,
			0xDFCF89, 0xC8DE8E, 0xB7E5B1, 0xA5E6C9, 0x99E2EB, 0xB9B9B9, 0x000000, 0x000000
		};
};