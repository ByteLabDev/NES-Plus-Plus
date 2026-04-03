#include "PPU.h"
#include "Nes.h"
#include <cstring>
#include <iostream>

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

uint8_t PPU::vram_read(uint16_t addr) {
	return 0;
}

void PPU::step() {
	if (scanline == 241 && cycles == 1) {
		status.flag_vblank = 1;
		if (ctrl.vblank_nmi_enable) {
			// https://www.nesdev.org/wiki/PPU_registers#Vblank_NMI
			// "Enabling NMI in PPUCTRL causes the NMI handler to be called at the start of vblank (scanline 241, dot 1)."
			nes->cpu6502.NMI();
		}
	}

	if (scanline == 261 && cycles == 1) {
		status.flag_vblank = 0;
	}

	cycles++;

	if (cycles < 341) return;

	cycles = 0;
	scanline++;
	int max_scanlines = (nes->region == nes->PAL) ? 312 : 262;

	if (scanline >= max_scanlines) {
		scanline = 0;
	}
}

bool PPU::read(uint16_t addr, uint8_t &data) {
	uint16_t reg = addr % 8; // Mirrors every 8 bytes from $2008 to $3FFF
	switch (reg) {
		case 0x2:	// PPUSTATUS
			data = status.reg;
			status.flag_vblank = 0;
			addr_latch = false;
			break;
		case 0x4:	// OAMDATA
			//data = oam_data;
			break;
		case 0x7:	// PPUDATA
			data = vram_read_buffer;
			vram_read_buffer = vram_read(vram_addr);
			vram_addr += (ctrl.increment_mode ? 32 : 1); // Increment by bit 2 of $2000 (0: add 1, going across; 1: add 32, going down)

			if (vram_addr >= 0x3F00 && vram_addr <= 0x3FFF) { // https://www.nesdev.org/wiki/PPU_registers#Reading_palette_RAM
				data = vram_read_buffer;
			}

			vram_addr = vram_addr % 0x3FFF; // The PPU address space is 14-bit, spanning $0000–$3FFF.
			break;
	}

	return true;
}

bool PPU::write(uint16_t addr, uint16_t data) {
	uint16_t reg = addr % 8; // Mirrors every 8 bytes from $2008 to $3FFF

	std::cout << "Addr: " << addr << std::endl;

	switch (reg) {
		case 0x0:	// PPUCTRL
			ctrl.reg = data;
			tmp_vram_addr &= !(0x0C00); // 0000 1100 0000 0000
			tmp_vram_addr |= (data & 0b11) << 10;
			break;
		case 0x1:	// PPUMASK
			mask.reg = data;
			break;
		case 0x3:	// OAMADDR
			oam_addr = data;
			break;
		case 0x4:	// OAMDATA
			break;
		case 0x5:	// PPUSCROLL
			break;
		case 0x6:	// PPUADDR
			break;
		case 0x7:	// PPUDATA
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