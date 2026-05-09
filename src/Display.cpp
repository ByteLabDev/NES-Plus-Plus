// src/Display.cpp

#include "Display.h"
#include "Nes.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

Display::Display(Nes* nesPtr) {
    nes = nesPtr;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    m_window = SDL_CreateWindow("Nes Plus Plus", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_SetWindowAspectRatio(m_window, 256.0f / 240.0f, 256.0f / 240.0f);
    m_renderer = SDL_CreateRenderer(m_window, NULL);
    nes_texture = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    debug_nt_texture = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    // Disable anti-aliasing
    SDL_SetTextureScaleMode(nes_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(debug_nt_texture, SDL_SCALEMODE_NEAREST);

    // Initialize ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer3_Init(m_renderer);
}

void Display::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) m_running = false;
    }

    // Start ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Menu bar functions
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load ROM", "Ctrl+O")) {
                open_file_dialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) m_running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Options", NULL, &show_options_menu));
            if (ImGui::MenuItem("Hard Reset")) {
                nes->cpu6502.hard_reset();
                nes->ppu.hard_reset();
                nes->apu.hard_reset();
            }
            if (ImGui::MenuItem("Soft Reset")) {
                nes->cpu6502.soft_reset();
                nes->ppu.soft_reset();
                nes->apu.soft_reset();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Nametable Viewer", NULL, &show_nt_debugger);
            ImGui::MenuItem("Sound Debugger", NULL, &show_sound_debugger);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Display::update_texture() {
    SDL_UpdateTexture(nes_texture, NULL, nes->ppu.frame_buffer, 256 * sizeof(uint32_t));
}

void Display::render() {
    draw_options_menu();
    draw_debug_windows();
    draw_sound_debugger();
    update_texture();
    ImGui::Render();

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    int window_w, window_h;
    SDL_GetWindowSize(m_window, &window_w, &window_h);

    const int menu_height = 20; // MainMenuBar height

    float aspect = 256.0f / 240.0f;

    SDL_FRect fdest;
    fdest.w = (float)window_w;
    fdest.h = (float)window_w / aspect;
    fdest.x = 0;
    fdest.y = menu_height + ((window_h - menu_height) - fdest.h) / 2.0f;

    SDL_RenderTexture(m_renderer, nes_texture, NULL, &fdest);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);

    SDL_RenderPresent(m_renderer);
}

void SDLCALL file_callback(void* userdata, const char* const* filelist, int filter) {
    Nes* nes = (Nes*)userdata;

    if (filelist && filelist[0]) {
        const char* selected_path = filelist[0];
        nes->cartridge->load_rom(selected_path);
    }
}

void Display::open_file_dialog() {
    const SDL_DialogFileFilter filters[] = {
        { "NES ROMs", "nes" },
        { "All Files", "*" }
    };

    SDL_ShowOpenFileDialog(file_callback, nes, m_window, filters, 2, NULL, false);
}

void Display::draw_options_menu() {
    if (!show_options_menu) return;

    ImGui::Begin("Options", &show_options_menu);

    const char* region_names[] = { "NTSC", "PAL" };
    static int selected_region_index = (nes->region == Common::NTSC) ? 0 : 1;

    if (ImGui::Combo("Region", &selected_region_index, region_names, IM_ARRAYSIZE(region_names)))
    {
        Common::Region new_region = (selected_region_index == 0) ? Common::NTSC : Common::PAL;

        nes->set_region(new_region);
        nes->apu.set_region(new_region);
    }

    ImGui::End();
}

void Display::draw_debug_windows() {
    if (show_nt_debugger) {
        // Update the pixel buffer based on the selected nametable
        nes->ppu.debug_render_nametable(nt_buffer, selected_nt, &hovered_attr_id);
        SDL_UpdateTexture(debug_nt_texture, NULL, nt_buffer, 256 * sizeof(uint32_t));

        ImGui::Begin("Nametable Debugger", &show_nt_debugger);

        // --- Selector ---
        ImGui::Text("Select Nametable:");
        for (int i = 0; i < 4; i++) {
            char label[16];
            sprintf(label, "NT %d", i);
            if (ImGui::RadioButton(label, &selected_nt, i)) {
                // Radio button logic handles updating selected_nt
            }
            if (i < 3) ImGui::SameLine();
        }

        ImGui::Separator();

        // --- Nametable Image ---
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2(512, 480); // Displayed at 2x scale
        ImGui::Image((ImTextureID)debug_nt_texture, ImVec2(canvas_size.x, canvas_size.y));

        // --- Attribute Indicator (Hover Logic) ---
        if (ImGui::IsItemHovered()) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            int rel_x = (int)(mouse_pos.x - canvas_pos.x) / 16; // Scale down from 512 to 32 tiles
            int rel_y = (int)(mouse_pos.y - canvas_pos.y) / 16; // Scale down from 480 to 30 tiles

            if (rel_x >= 0 && rel_x < 32 && rel_y >= 0 && rel_y < 30) {
                // Manually calculate the attribute ID for the hovered coordinate
                uint16_t nt_base = 0x2000 + (selected_nt * 0x400);
                uint16_t attr_addr = nt_base + 960 + ((rel_y / 4) * 8) + (rel_x / 4);
                uint8_t attr_byte = nes->ppu.vram_read(attr_addr);
                uint8_t pal_id = (attr_byte >> (((rel_y & 2) ? 4 : 0) + ((rel_x & 2) ? 2 : 0))) & 0x03;

                ImGui::BeginTooltip();
                ImGui::Text("Tile: (%d, %d)", rel_x, rel_y);
                ImGui::Text("Attr Addr: 0x%04X", attr_addr);
                ImGui::Text("Palette ID: %d", pal_id);
                ImGui::EndTooltip();
            }
        }

        ImGui::End();
    }
}


void Display::draw_sound_debugger() {
    if (!show_sound_debugger) return;

    ImGui::Begin("Sound Debugger", &show_sound_debugger);

    auto& dbg = nes->apu.debug;

    // --- Master Controls ---
    ImGui::Text("Channel Toggles:");
    ImGui::Checkbox("Pulse 1", &dbg.p1_enabled); ImGui::SameLine();
    ImGui::Checkbox("Pulse 2", &dbg.p2_enabled); ImGui::SameLine();
    ImGui::Checkbox("Triangle", &dbg.tri_enabled);
    ImGui::Checkbox("Noise", &dbg.noise_enabled);
    ImGui::Checkbox("DMC", &dbg.dmc_enabled);

    ImGui::Separator();

    // --- Waveform Visualization ---
    // We use an overlay to show the "rolling" nature of the buffer
    ImGui::Text("Pulse 1");
    ImGui::PlotLines("##p1", dbg.p1_history, 100, dbg.write_idx, NULL, 0.0f, 15.0f, ImVec2(0, 50));

    ImGui::Text("Pulse 2");
    ImGui::PlotLines("##p2", dbg.p2_history, 100, dbg.write_idx, NULL, 0.0f, 15.0f, ImVec2(0, 50));

    ImGui::Text("Triangle");
    ImGui::PlotLines("##tri", dbg.tri_history, 100, dbg.write_idx, NULL, 0.0f, 15.0f, ImVec2(0, 50));

    ImGui::Text("DMC");
    ImGui::PlotLines("##dmc", dbg.dmc_history, 100, dbg.write_idx, NULL, 0.0f, 127.0f, ImVec2(0, 50));

    ImGui::Text("Noise");
    ImGui::PlotLines("##dmc", dbg.noise_history, 100, dbg.write_idx, NULL, 0.0f, 15.0f, ImVec2(0, 50));

    ImGui::Separator();
    ImGui::Text("Final Mixer Output");
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green for mixed
    ImGui::PlotLines("##mixed", dbg.mixed_history, 100, dbg.write_idx, NULL, 0.0f, 1.0f, ImVec2(0, 80));
    ImGui::PopStyleColor();

    ImGui::End();
}