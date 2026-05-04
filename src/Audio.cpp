// src/Audio.cpp
#define _USE_MATH_DEFINES

#include "Audio.h"
#include "SDL3/SDL.h"
#include "Nes.h"
#include <math.h>
#include <iostream>

Audio::Audio(Nes* nesPtr) {
    nes = nesPtr;
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;

    dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    stream = SDL_CreateAudioStream(&spec, &spec);

    SDL_BindAudioStream(dev, stream);
    SDL_ResumeAudioDevice(dev);
}

void Audio::push_samples(const float* data, int count) {
    SDL_PutAudioStreamData(stream, data, count * sizeof(float));
}