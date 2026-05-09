// src/PPU.cpp

#include "PPU.h"
#include "Nes.h"
#include <cstring>
#include <iostream>
#include "Cartridge.h"
#include "Common.h"

using namespace Common;

PPU::PPU(Nes* nesPtr) {
	nes = nesPtr;
	ctrl.reg = 0x0;
	mask.reg = 0x0;
	status.reg = 0x0;

	vram_addr = 0x0;
	tmp_vram_addr = 0x0;
	fine_x = 0x0;
	addr_latch = false;

	vram_read_buffer = 0x0;

	oam_addr = 0x0;

	std::memset(oam_data, 0x00, sizeof(oam_data));
}

uint16_t PPU::get_mirror_index(uint16_t addr) {
	// $3000-3EFF --> 2000-2FFF
	uint16_t normalized_addr = (addr & 0x2FFF) - 0x2000;
	Cartridge::MirrorMode mode = nes->cartridge->get_mirror_mode();
	if (mode == Cartridge::MirrorMode::VERTICAL) {
		return normalized_addr % 0x800;
	}
	else if (mode == Cartridge::MirrorMode::HORIZONTAL) {
		if (normalized_addr < 0x800) {
			return normalized_addr % 0x400;
		}
		else {
			return (normalized_addr % 0x400) + 0x400;
		}
	}
	else if (mode == Cartridge::MirrorMode::FOUR_SCREEN) {
		return normalized_addr;
	}
	return 0;
}

uint8_t PPU::vram_read(uint16_t addr) {
	if (addr < 0x2000) {
		uint8_t data = 0;
		if (nes->cartridge != nullptr) {
			nes->cartridge->read(addr, data);
		}
		return data;
	}
	else if (addr < 0x3F00) {
		return vram[get_mirror_index(addr)];
	}
	else {
		addr &= 0x001F;
		if (addr == 0x0010) addr = 0x0000;
		if (addr == 0x0014) addr = 0x0004;
		if (addr == 0x0018) addr = 0x0008;
		if (addr == 0x001C) addr = 0x000C;
		return palette_ram[addr];
	}
	return 0;
}

void PPU::vram_write(uint16_t addr, uint8_t data) {
	addr &= 0x3FFF;

	// CHR-ROM / CHR-RAM ($0000 - $2000)
	if (addr < 0x2000 && nes->cartridge != nullptr) {
		nes->cartridge->write(addr, data);
		return;
	}

	// Nametables and Mirrors ($2000 - $3F00)
	if (addr < 0x3F00) {
		vram[get_mirror_index(addr)] = data;
		return;
	}

	// Palettes ($3F00 - $3FFF)
	uint16_t palette_addr = addr & 0x1F;
	if (palette_addr == 0x10) palette_addr = 0x00;
	else if (palette_addr == 0x14) palette_addr = 0x04;
	else if (palette_addr == 0x18) palette_addr = 0x08;
	else if (palette_addr == 0x1C) palette_addr = 0x0C;
	palette_ram[palette_addr] = data;

	return;
}

void PPU::step() {
	c_count_temp++;
	cycles++;

	handle_counters();

	if (scanline < 240 && cycles >= 1 && cycles <= 256) {
		render_pixel();
	}

	bool rendering_enabled = mask.render_bg || mask.render_sp;

	// Visible scanlines (0-239) + Pre-render scanline (261)
	if (rendering_enabled && (scanline < 240 || scanline == 261)) {
		process_visible_scanline();
	}

	// Scanlines 241-260 | Vertical blanking lines
	if (scanline >= 241 && scanline <= 260) {

	}
}

void PPU::handle_counters() {
	if (cycles >= 341) {
		cycles = 0;
		scanline++;

		evaluate_sprites();

		int max_scanlines = (nes->region == Region::PAL) ? 312 : 262;

		if (scanline >= max_scanlines) {
			scanline = 0;
		}
	}

	if (scanline == 241 && cycles == 1) {
		status.flag_vblank = 1;
		frame_ready = true;
		if (ctrl.vblank_nmi_enable) {
			// https://www.nesdev.org/wiki/PPU_registers#Vblank_NMI
			// "Enabling NMI in PPUCTRL causes the NMI handler to be called at the start of vblank (scanline 241, dot 1)."
			nes->cpu6502.NMI();
		}
	}

	if (scanline == 261 && cycles == 1) {
		status.flag_vblank = 0;
		status.flag_sp_0_hit = 0;
		status.flag_sp_overflow = 0;
	}
}

void PPU::process_visible_scanline() {
	if (cycles == 0) {
		return;
	}

	// Cycles	1-256		|	Data is fetched for each tile
	// Cycles	321-336		|	Prefetch cycles
	if (cycles >= 1 && cycles <= 256 || (cycles >= 321 && cycles <= 336)) {
		update_shift_registers();

		uint8_t step = (cycles - 1) % 8;

		switch (step) {
		case 1:
			// tile address = 0x2000 | (v & 0x0FFF)
			bg_next_tile_id = vram_read(0x2000 | (vram_addr & 0x0FFF));
			break;
		case 3: {
			// attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)
			uint16_t bg_next_attrib_addr = 0x23C0 | (vram_addr & 0x0C00) | ((vram_addr >> 4) & 0x38) | ((vram_addr >> 2) & 0x07);
			bg_next_attrib = vram_read(bg_next_attrib_addr);

			if (vram_addr & 0x0040) bg_next_attrib >>= 4;
			if (vram_addr & 0x0002) bg_next_attrib >>= 2;

			bg_next_attrib &= 0x03;
			break;
		}
		case 5:
			bg_next_lo = vram_read((ctrl.bg_pattern << 12) | (bg_next_tile_id << 4) | ((vram_addr >> 12) & 0x07));
			break;
		case 7:
			bg_next_hi = vram_read((ctrl.bg_pattern << 12) | (bg_next_tile_id << 4) | (((vram_addr >> 12) & 0x07) + 8));
			coarse_x_increment();
			load_shift_registers();
			break;
		}
	}

	if (cycles == 256) {
		y_increment();
	}

	if (cycles == 257) {
		transfer_x_address();
	}

	if (scanline == 261 && cycles >= 280 && cycles <= 304) {
		transfer_y_address();
	}

	// Cycles	257-320		|	The tile data for the sprites on the next scanline are fetched here
	if (cycles >= 257 && cycles <= 320) {
		uint8_t sprite_index = (cycles - 257) / 8;
		Sprite& s = sprite_buffer[sprite_index];

		if (s.y >= 240) return; // Don't render sprite if it's off-screen

		uint8_t step = (cycles - 1) % 8;
		uint8_t row = scanline - s.y;

		// Flip the sprite vertically if bit 7 is set
		if (s.attr & 0x80) {
			uint8_t height = ctrl.sprite_size ? 15 : 7;
			row = height - row;
		}

		uint16_t tile_addr;
		if (!ctrl.sprite_size) {
			tile_addr = (ctrl.sp_pattern << 12) | (s.id << 4) | row;	// Use 8x8 sprite ($2000)
		}
		else {
			tile_addr = ((s.id & 0x01) << 12) | ((s.id & 0xFE) << 4) | (row & 0x07); // Use 8x16 sprite ($2000)
			if (row & 0x08) tile_addr += 16; // Move to second tile of sprite
		}
		if (row & 0x08) tile_addr += 16;

		switch (step) {
			case 5: {
				s.shifter_lo = vram_read(tile_addr);
				break;
			}

			case 7: {
				s.shifter_hi = vram_read(tile_addr + 8); // Add 8 to indicate bit plane = 1 (P)

				if (s.attr & 0x40) {
					auto flip = [](uint8_t b) {
						b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
						b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
						b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
						return b;
						};
					s.shifter_lo = flip(s.shifter_lo);
					s.shifter_hi = flip(s.shifter_hi);
				}
				break;
			}
		}
	}
}

void PPU::evaluate_sprites() {
	sprite_count = 0;
	for (int i = 0; i < 8; i++) {
		sprite_buffer[i] = default_sp;
		sprite_buffer[i].shifter_lo = 0; // Ensure shifters are reset
		sprite_buffer[i].shifter_hi = 0;
	}

	int sprite_height = ctrl.sprite_size ? 16 : 8;

	for (int i = 0; i < 64 && sprite_count < 8; i++) {
		uint8_t sprite_y = oam_data[i * 4];
		int diff = scanline - (int)sprite_y;

		if (diff >= 0 && diff < sprite_height) {
			Sprite& s = sprite_buffer[sprite_count];
			s.id = oam_data[i * 4 + 1];
			s.attr = oam_data[i * 4 + 2];
			s.x = oam_data[i * 4 + 3];
			s.y = sprite_y;
			s.is_sprite_zero = (i == 0);

			// --- IMMEDIATE FETCH LOGIC ---
			uint8_t row = diff;
			if (s.attr & 0x80) row = (sprite_height - 1) - row;

			uint16_t tile_addr;
			if (!ctrl.sprite_size) {
				tile_addr = (ctrl.sp_pattern << 12) | (s.id << 4) | (row & 0x07);
			}
			else {
				tile_addr = ((s.id & 0x01) << 12) | ((s.id & 0xFE) << 4) | (row & 0x07);
				if (row & 0x08) tile_addr += 16;
			}

			s.shifter_lo = vram_read(tile_addr);
			s.shifter_hi = vram_read(tile_addr + 8);

			// Horizontal Flip
			if (s.attr & 0x40) {
				auto flip = [](uint8_t b) {
					b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
					b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
					b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
					return b;
					};
				s.shifter_lo = flip(s.shifter_lo);
				s.shifter_hi = flip(s.shifter_hi);
			}

			sprite_count++;
		}
	}
}

void PPU::transfer_x_address() {
	// If rendering is enabled, the PPU copies all bits related to horizontal position from t to v:
	// v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF 0b000010000011111
	vram_addr = (vram_addr & ~0x41F) | (tmp_vram_addr & 0x41F);
}

void PPU::transfer_y_address() {
	// If rendering is enabled, at the end of vblank, shortly after the horizontal bits are copied from t to v at dot 257, 
	// the PPU will repeatedly copy the vertical bits from t to v from dots 280 to 304, completing the full initialization of v from t: 
	// v: GHIA.BC DEF..... <- t: GHIA.BC DEF..... 0b111101111100000
	vram_addr = (vram_addr & ~0x7BE0) | (tmp_vram_addr & 0x7BE0);
}

bool PPU::read(uint16_t addr, uint8_t& data) {
	uint16_t reg = addr % 8; // Mirrors every 8 bytes from $2008 to $3FFF
	switch (reg) {
	case 0x2:	// PPUSTATUS
		data = status.reg;
		status.flag_vblank = 0;
		addr_latch = false;
		break;
	case 0x4:	// OAMDATA
		data = oam_data[oam_addr];
		open_bus = data;
		break;
	case 0x7:	// PPUDATA
		data = vram_read_buffer;
		vram_read_buffer = vram_read(vram_addr);

		if (vram_addr >= 0x3F00 && vram_addr <= 0x3FFF) { // https://www.nesdev.org/wiki/PPU_registers#Reading_palette_RAM
			data = vram_read_buffer;
		}

		open_bus = data;
		vram_addr += (ctrl.increment_mode ? 32 : 1); // Increment by bit 2 of $2000 (0: add 1, going across; 1: add 32, going down)
		vram_addr &= 0x3FFF; // The PPU address space is 14-bit, spanning $0000–$3FFF.
		break;
	default:
		data = open_bus;
		break;
	}

	open_bus = data;

	return true;
}

bool PPU::write(uint16_t addr, uint8_t data) {
	open_bus = data;

	uint16_t reg = addr % 8; // Mirrors every 8 bytes from $2008 to $3FFF
	switch (reg) {
	case 0x0:	// PPUCTRL
		ctrl.reg = data;
		tmp_vram_addr &= ~(0x0C00); // 0000 1100 0000 0000
		tmp_vram_addr |= (data & 0b11) << 10;
		break;
	case 0x1:	// PPUMASK
		mask.reg = data;
		break;
	case 0x3:	// OAMADDR
		oam_addr = data;
		break;
	case 0x4:	// OAMDATA
		oam_data[oam_addr++] = (uint8_t)(data & 0xFF);
		break;
	case 0x5:	// PPUSCROLL
		if (!addr_latch) {
			uint16_t new_data = ((uint16_t)data & 0xF8) >> 3;
			tmp_vram_addr = (tmp_vram_addr & ~0x1F) | new_data;
			fine_x = data & 0x7;
			addr_latch = true;
		}
		else {
			tmp_vram_addr &= ~0x73E0;
			tmp_vram_addr |= (data & 0x07) << 12; // Fine Y
			tmp_vram_addr |= (data & 0xF8) << 2;  // Coarse Y
			addr_latch = false;
		}
		break;
	case 0x6:	// PPUADDR
		if (!addr_latch) {
			tmp_vram_addr = (tmp_vram_addr & 0x00FF) | ((data & 0x3F) << 8);
			addr_latch = true;
		}
		else {
			tmp_vram_addr = (tmp_vram_addr & 0xFF00) | (data & 0xFF);
			vram_addr = tmp_vram_addr;
			addr_latch = false;
		}
		break;
	case 0x7:	// PPUDATA
		vram_write(vram_addr, (uint8_t)(data & 0xFF));
		vram_addr += (ctrl.increment_mode ? 32 : 1);
		vram_addr &= 0x3FFF;
		break;
	}

	return true;
}

void PPU::soft_reset() {
	ctrl.reg = 0x0;
	mask.reg = 0x0;
	status.reg = status.reg & 0b10000000;

	vram_addr = 0x0;
	tmp_vram_addr = 0x0;
	fine_x = 0x0;
	addr_latch = false;

	vram_read_buffer = 0x0;

	cycles = 0;
	scanline = 0;
	frame_ready = false;
}

void PPU::hard_reset() {
	soft_reset();
	
	oam_addr = 0;
	memset(oam_data, 0, sizeof(oam_data));
	memset(secondary_oam_data, 0, sizeof(secondary_oam_data));

	memset(vram, 0, sizeof(vram));
	memset(palette_ram, 0, sizeof(palette_ram));

	bg_next_tile_id = 0;		
	bg_next_attrib = 0;			
	bg_next_lo = 0;				
	bg_next_hi = 0;				

	bg_shifter_pattern_lo = 0;	
	bg_shifter_pattern_hi = 0;	
	bg_shifter_attrib_lo = 0;	
	bg_shifter_attrib_hi = 0;	

	memset(sprite_buffer, 0, sizeof(sprite_buffer));
	sprite_count = 0;			

	open_bus = 0x00;
}

void PPU::coarse_x_increment() {
	if ((vram_addr & 0x001F) == 31) {
		vram_addr &= ~0x001F;
		vram_addr ^= 0x0400;
	}
	else {
		vram_addr++;
	}
}

void PPU::y_increment() {
	if ((vram_addr & 0x7000) != 0x7000) {
		vram_addr += 0x1000;
		return;
	}

	vram_addr &= ~0x7000;
	int y = (vram_addr & 0x03E0) >> 5;
	if (y == 29) {
		y = 0;
		vram_addr ^= 0x0800;
	}
	else if (y == 31) {
		y = 0;
	}
	else {
		y++;
	}

	vram_addr = (vram_addr & ~0x03E0) | (y << 5);
}

void PPU::load_shift_registers() {
	// Move the bottom 8 bits to the top 8 bits, and put the new data in the bottom
	bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_lo;
	bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_hi;

	// Do the same for attributes
	bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_attrib & 0x01) ? 0xFF : 0x00);
	bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_attrib & 0x02) ? 0xFF : 0x00);
}

void PPU::update_shift_registers() {
	if (mask.render_bg || mask.render_sp) {
		bg_shifter_pattern_lo <<= 1;
		bg_shifter_pattern_hi <<= 1;
		bg_shifter_attrib_lo <<= 1;
		bg_shifter_attrib_hi <<= 1;
	}

	if (mask.render_sp && cycles >= 1 && cycles <= 256) {
		for (int i = 0; i < sprite_count; i++) {
			if (sprite_buffer[i].x > 0) {
				sprite_buffer[i].x--;
			}
			else {
				sprite_buffer[i].shifter_lo <<= 1;
				sprite_buffer[i].shifter_hi <<= 1;
			}
		}
	}
}

void PPU::render_pixel() {
	int x = cycles - 1;
	int y = scanline;
	if (x < 0 || x >= 256 || y < 0 || y >= 240) return;

	// --- 1. Background Pixel Logic ---
	uint16_t bit_mux = 0x8000 >> fine_x;
	uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
	uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
	uint8_t pixel_val = (p1 << 1) | p0;

	uint8_t a0 = (bg_shifter_attrib_lo & bit_mux) > 0;
	uint8_t a1 = (bg_shifter_attrib_hi & bit_mux) > 0;
	uint8_t palette_id = (a1 << 1) | a0;

	// --- 2. Sprite Pixel Logic ---
	uint8_t fg_pixel = 0x00;
	uint8_t fg_palette = 0x00;
	uint8_t fg_priority = 0x00;
	int found_sprite_index = -1;

	if (mask.render_sp) {
		for (int i = 0; i < sprite_count; i++) {
			// Only consider sprites whose X counter has reached 0
			if (sprite_buffer[i].x == 0) {
				uint8_t pixel_lo = (sprite_buffer[i].shifter_lo & 0x80) > 0;
				uint8_t pixel_hi = (sprite_buffer[i].shifter_hi & 0x80) > 0;
				fg_pixel = (pixel_hi << 1) | pixel_lo;

				if (fg_pixel != 0) { // Sprite is not transparent here
					fg_palette = (sprite_buffer[i].attr & 0x03) + 0x04; // Use sprite palettes (4-7)
					fg_priority = (sprite_buffer[i].attr & 0x20) > 0;
					found_sprite_index = i;
					break; // First visible sprite in OAM wins
				}
			}
		}
	}

	// --- 3. Multiplexing (Decision) ---
	uint8_t final_pixel = 0x00;
	uint8_t final_palette = 0x00;

	if (pixel_val == 0 && fg_pixel == 0) {
		final_pixel = 0; final_palette = 0;
	}
	else if (pixel_val == 0 && fg_pixel > 0) {
		final_pixel = fg_pixel; final_palette = fg_palette;
	}
	else if (pixel_val > 0 && fg_pixel == 0) {
		final_pixel = pixel_val; final_palette = palette_id;
	}
	else {
		// Both exist: check priority
		if (fg_priority == 0) {
			final_pixel = fg_pixel; final_palette = fg_palette;
		}
		else {
			final_pixel = pixel_val; final_palette = palette_id;
		}

		// Sprite 0 Hit Logic
		if (mask.render_bg && mask.render_sp) {
			if (sprite_buffer[found_sprite_index].is_sprite_zero && x < 255) {
				status.flag_sp_0_hit = 1;
			}
		}
	}

	uint16_t final_addr = 0x3F00 + (final_palette * 4) + final_pixel;
	uint8_t final_color_index = vram_read(final_addr);
	frame_buffer[y * 256 + x] = system_palette[final_color_index & 0x3F] | 0xFF000000;
}

void PPU::debug_render_nametable(uint32_t* buffer, int nt_index, int* mouse_attr_id) {
	// A nametable is 32x30 tiles. 1 tile = 8x8 pixels. Total = 256x240 pixels.
	uint16_t nt_base = 0x2000 + (nt_index * 0x400);

	for (int y = 0; y < 30; y++) {
		for (int x = 0; x < 32; x++) {
			// 1. Get Tile ID
			uint8_t tile_id = vram_read(nt_base + y * 32 + x);

			// 2. Get Attribute (Palette)
			// Attributes start 960 bytes after NT base
			uint16_t attrib_addr = nt_base + 960 + ((y / 4) * 8) + (x / 4);
			uint8_t attrib_byte = vram_read(attrib_addr);

			// Extract the 2 bits for this specific 16x16 quadrant
			uint8_t palette_id = (attrib_byte >> (((y & 2) ? 4 : 0) + ((x & 2) ? 2 : 0))) & 0x03;

			// 3. Render the 8x8 tile pixels
			for (int row = 0; row < 8; row++) {
				uint8_t low = vram_read((ctrl.bg_pattern << 12) | (tile_id << 4) | row);
				uint8_t high = vram_read((ctrl.bg_pattern << 12) | (tile_id << 4) | row + 8);

				for (int col = 0; col < 8; col++) {
					uint8_t pixel = ((low >> (7 - col)) & 0x01) | (((high >> (7 - col)) & 0x01) << 1);

					uint16_t p_addr = 0x3F00 + (palette_id * 4) + pixel;
					uint8_t color_idx = vram_read(p_addr) & 0x3F;

					int px = x * 8 + col;
					int py = y * 8 + row;
					buffer[py * 256 + px] = system_palette[color_idx] | 0xFF000000;
				}
			}
		}
	}
}