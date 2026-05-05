// src/Controller.cpp

// https://www.nesdev.org/wiki/Standard_controller

#include "Controller.h"
#include "Nes.h"
#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <iostream>

Controller::Controller(Nes* nesPtr) {
	nes = nesPtr;
	SDL_InitSubSystem(SDL_INIT_GAMEPAD);

	int instance_count;
	SDL_JoystickID* joysticks = SDL_GetGamepads(&instance_count);
	if (instance_count > 0) {
		gamepad = SDL_OpenGamepad(joysticks[0]);
	}
	SDL_free(joysticks);
}

void Controller::update_state() {
	int numkeys;
	const bool* state = SDL_GetKeyboardState(&numkeys);

	uint8_t current_buttons = 0x00;
	if (state[SDL_SCANCODE_X])      current_buttons |= (1 << 0); // A
	if (state[SDL_SCANCODE_Z])      current_buttons |= (1 << 1); // B
	if (state[SDL_SCANCODE_A])      current_buttons |= (1 << 2); // Select
	if (state[SDL_SCANCODE_S])      current_buttons |= (1 << 3); // Start
	if (state[SDL_SCANCODE_UP])     current_buttons |= (1 << 4); // Up
	if (state[SDL_SCANCODE_DOWN])   current_buttons |= (1 << 5); // Down
	if (state[SDL_SCANCODE_LEFT])   current_buttons |= (1 << 6); // Left
	if (state[SDL_SCANCODE_RIGHT])  current_buttons |= (1 << 7); // Right

	if (gamepad && SDL_GamepadConnected(gamepad)) {
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH))     current_buttons |= (1 << 0); // A
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST))      current_buttons |= (1 << 1); // B
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK))      current_buttons |= (1 << 2); // Select
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START))     current_buttons |= (1 << 3); // Start
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))   current_buttons |= (1 << 4);
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) current_buttons |= (1 << 5);
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) current_buttons |= (1 << 6);
		if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))current_buttons |= (1 << 7);

		// Map D-Pad to Left Analog Stick
		if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) < -16000) current_buttons |= (1 << 4);
		if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) > 16000)  current_buttons |= (1 << 5);
		if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) < -16000) current_buttons |= (1 << 6);
		if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) > 16000)  current_buttons |= (1 << 7);
	}

	controller_state = current_buttons;
}

uint8_t Controller::read() {
	uint8_t value;

	if (strobe) {
		value = (controller_state & 0x01);
	} else {
		value = (shift_register & 0x01);
		shift_register >>= 1;
		shift_register |= 0x80;
	}

	return value;
}

void Controller::write(uint8_t data) {
	strobe = (data & 0x01);
	if (strobe) {
		shift_register = controller_state;
	}
}