#include "Display.h"
#include "Nes.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

Display::Display(Nes* nesPtr) {
    nes = nesPtr;
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Nes Plus Plus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    nes_texture = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    // Initialize ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);
}

void Display::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) m_running = false;
    }

    // Start ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Menu bar functions
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) m_running = false;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Display::update_texture() {
    SDL_UpdateTexture(nes_texture, NULL, nes->ppu.frame_buffer, 256 * sizeof(uint32_t));
}

void Display::render() {
    update_texture();
    ImGui::Render();

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    int window_w, window_h;
    SDL_GetWindowSize(m_window, &window_w, &window_h);

    // Account for the menu bar height
    int menu_height = 20;
    int available_h = window_h - menu_height;

    // Lock aspect ratio
    float target_aspect = 256.0f / 240.0f;
    float window_aspect = (float)window_w / (float)available_h;

    SDL_Rect dest;

    if (window_aspect > target_aspect) {
        // Window is too wide (Pillarboxing)
        dest.h = available_h;
        dest.w = (int)(available_h * target_aspect);
        dest.x = (window_w - dest.w) / 2;
        dest.y = menu_height;
    }
    else {
        // Window is too tall (Letterboxing)
        dest.w = window_w;
        dest.h = (int)(window_w / target_aspect);
        dest.x = 0;
        dest.y = menu_height + (available_h - dest.h) / 2;
    }

    // Render image
    SDL_RenderCopy(m_renderer, nes_texture, NULL, &dest);

    // Draw menu bar
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);

    SDL_RenderPresent(m_renderer);
}