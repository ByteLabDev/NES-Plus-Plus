#pragma once
#include <SDL.h>

class Display {
public:
    Display();
    //~Display();
    void update();
    void render();
    bool isOpen() const { return m_running; }

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_running = true;
};