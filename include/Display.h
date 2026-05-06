// include/Display.h

#pragma once
#include <SDL3/SDL.h>

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
    void draw_debug_windows();
    void open_file_dialog();

private:
    Nes* nes;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* nes_texture;
    bool m_running = true;

    // Options menu
    bool show_options_menu = false;
    void draw_options_menu();

    // Nametable debugging
    SDL_Texture* debug_nt_texture = nullptr;
    uint32_t nt_buffer[256 * 240];
    bool show_nt_debugger = false;
    int selected_nt = 0;
    int hovered_attr_id = -1;

    // Sound debugging
    bool show_sound_debugger = false;
    void draw_sound_debugger();
};