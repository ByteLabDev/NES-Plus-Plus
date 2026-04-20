#include "PPU.h"
#include "Nes.h"
#include <cstring>
#include <iostream>
#include "Cartridge.h"

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
	palette_ram[palette_addr] = data;

	return;
}

void PPU::step() {
	c_count_temp++;
	cycles++;

	if (cycles >= 341) {
		cycles = 0;
		scanline++;
		int max_scanlines = (nes->region == nes->PAL) ? 312 : 262;

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

	if (scanline < 240 && cycles >= 1 && cycles <= 256) {
		render_pixel();
	}

	if (scanline == 261 && cycles == 1) {
		status.flag_vblank = 0;
	}
}

bool PPU::read(uint16_t addr, uint8_t &data) {
	//std::cout << "READ: 0x" << std::hex << addr << std::endl;
	uint16_t reg = addr % 8; // Mirrors every 8 bytes from $2008 to $3FFF
	switch (reg) {
		case 0x2:	// PPUSTATUS
			data = status.reg;
			status.flag_vblank = 0;
			addr_latch = false;
			break;
		case 0x4:	// OAMDATA
			data = oam_data[oam_addr];
			break;
		case 0x7:	// PPUDATA
			data = vram_read_buffer;
			vram_read_buffer = vram_read(vram_addr);

			if (vram_addr >= 0x3F00 && vram_addr <= 0x3FFF) { // https://www.nesdev.org/wiki/PPU_registers#Reading_palette_RAM
				data = vram_read_buffer;
			}

			vram_addr += (ctrl.increment_mode ? 32 : 1); // Increment by bit 2 of $2000 (0: add 1, going across; 1: add 32, going down)

			vram_addr &= 0x3FFF; // The PPU address space is 14-bit, spanning $0000–$3FFF.
			break;
	}

	return true;
}

bool PPU::write(uint16_t addr, uint16_t data) {
	//std::cout << "WRITE: 0x" << std::hex << addr << std::endl;
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
				fine_x = data & 0x7;
				tmp_vram_addr = (tmp_vram_addr & 0xFFE0) | (data >> 3);
				addr_latch = true;
			}
			else {
				tmp_vram_addr = (tmp_vram_addr & 0x8C1F) | ((data & 0xF8) << 2);
				tmp_vram_addr = (tmp_vram_addr & 0x0FFF) | ((data & 0x07) << 12);
				addr_latch = false;
			}
			break;
		case 0x6:	// PPUADDR
			if (!addr_latch) {
				tmp_vram_addr = (tmp_vram_addr & 0x00FF) | ((data & 0x3F) << 8);
				addr_latch = true;
			} else {
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

void PPU::reset() {
	ctrl.reg = 0x0;
	mask.reg = 0x0;
	status.reg = status.reg & 0b10000000; // U??x xxxx

	fine_x = 0x0;
	addr_latch = false;

	vram_read_buffer = 0x0;
}

void PPU::render_pixel() {
	int x = cycles - 1;
	int y = scanline;

	uint16_t tile_x = x / 8;
	uint16_t tile_y = y / 8;
	uint16_t nametable_addr = 0x2000 + (tile_y * 32) + tile_x;
	uint8_t tile_id = vram_read(nametable_addr);

	uint16_t pattern_base = ctrl.bg_pattern ? 0x1000 : 0x0000;
	int fine_y = y % 8;
	int fine_x = x % 8;

	uint16_t addr_low = pattern_base + (tile_id * 16) + fine_y;
	uint16_t addr_high = addr_low + 8;

	uint8_t low_byte = vram_read(addr_low);
	uint8_t high_byte = vram_read(addr_high);

	int bit = 7 - fine_x;
	uint8_t pixel_val = ((low_byte >> bit) & 0x01) | (((high_byte >> bit) & 0x01) << 1);

	uint8_t color_index = vram_read(0x3F00 + pixel_val);

	frame_buffer[y * 256 + x] = system_palette[color_index & 0x3F] | 0xFF000000;
}