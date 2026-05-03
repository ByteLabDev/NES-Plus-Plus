// src/main.cpp

#define SDL_MAIN_HANDLED
#include "Nes.h"
#include "Bus.h"
#include "Cpu6502.h"
#include "Cartridge.h"
#include "Display.h"
#include "Controller.h"
#include <chrono>
#include <thread>
#include <iostream>

using namespace std::chrono_literals;

const std::chrono::nanoseconds frame_target_time(1000000000 / 60);

int main(int argc, const char* argv[]) {
	Nes nes;

	Cartridge cartridge = Cartridge();

	nes.insert_cartridge(&cartridge);

    auto last_frame_time = std::chrono::high_resolution_clock::now();

    while (nes.display.isOpen()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        if (elapsed >= frame_target_time) {
            last_frame_time = current_time;

            if (nes.cartridge->is_loaded) {
                nes.controller.update_state();
                nes.display.update();

                while (!nes.ppu.frame_ready) {
                    nes.tick();
                }
                nes.ppu.frame_ready = false;

                nes.display.render();
            }
            else {
                nes.display.update();
                nes.display.render();
            }

        }
        else {
            std::this_thread::yield();
        }
    }
    return 0;
}