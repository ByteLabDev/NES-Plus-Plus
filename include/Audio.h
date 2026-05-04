// include/Audio.h

#pragma once
#include <SDL3/SDL.h>

class Nes;

class Audio {
	public:
		Audio(Nes* nesPtr);
		void push_samples(const float* data, int count);
	private:
		Nes* nes;
		SDL_AudioDeviceID dev;
		SDL_AudioStream* stream;
};