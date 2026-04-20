#pragma once
#include <SDL.h>

class Nes;

class Display {
public:
    Display(Nes* nesPtr);
    //~Display();
    void update();
    void render();
    bool isOpen() const { return m_running; }
    void update_texture();
    void draw_debug_pattern_tables();

private:
    Nes* nes;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* nes_texture;
    bool m_running = true;
};