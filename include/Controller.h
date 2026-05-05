#pragma once
#include <cstdint>
#include <SDL3/Sdl.h>

class Nes;

class Controller {
public:
    Controller(Nes* nesPtr);
    void update_state();
    uint8_t read();
    void write(uint8_t data);

private:
    Nes* nes;
    SDL_Gamepad* gamepad = nullptr;
    uint8_t controller_state = 0;
    uint8_t shift_register = 0;
    bool strobe = false;
};