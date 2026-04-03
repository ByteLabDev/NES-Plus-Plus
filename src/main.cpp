// main.cpp

#include "Nes.h"
#include "Bus.h";
#include "Cpu6502.h"
#include "Cartridge.h"; // TEMP - REMOVE AFTER TESTING
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, const char* argv[]) {
	Nes nes;

	Cartridge cartridge = Cartridge("C:\\Users\\adems\\Downloads\\Bomberman.nes");
	cartridge.loadRom();

	nes.bus.insertCartridge(&cartridge);

	nes.cpu6502.reset();

	while (true) {
		nes.tick();
		std::this_thread::sleep_for(1ms);
	}

	return 0;
}