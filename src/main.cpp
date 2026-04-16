// main.cpp

#define SDL_MAIN_HANDLED
#include "Nes.h"
#include "Bus.h";
#include "Cpu6502.h"
#include "Cartridge.h"; // TEMP - REMOVE AFTER TESTING
#include "Display.h"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, const char* argv[]) {
	Nes nes;
	Display display;

	Cartridge cartridge = Cartridge("C:\\Users\\adems\\Downloads\\Bomberman.nes");
	cartridge.loadRom();

	nes.insert_cartridge(&cartridge);

	nes.cpu6502.reset();


	while (display.isOpen()) {
		display.update();
		nes.tick();
		display.render();
		//std::this_thread::sleep_for(1ms);
	}

	return 0;
}