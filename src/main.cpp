// main.cpp

#include "Bus.h";
#include "Cpu6502.h"
#include "Cartridge.h"; // TEMP - REMOVE AFTER TESTING
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, const char* argv[]) {
	PPU ppu;
	Bus bus(&ppu);
	Cpu6502 cpu6502(&bus);

	Cartridge cartridge = Cartridge("C:\\Users\\adems\\Downloads\\Super Mario Bros. (World).nes");
	cartridge.loadRom();

	bus.insertCartridge(&cartridge);

	cpu6502.reset();

	while (true) {
		cpu6502.step();
		std::this_thread::sleep_for(1ms);
	}

	return 0;
}