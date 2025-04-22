#include <borealis/platforms/sdl/sdl_audio.hpp>
#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "borealis/core/audio.hpp"
#include "borealis/core/logger.hpp"

#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#else
#include <cstddef>
#endif

namespace brls
{
#define SOUND_LOAD_UNLOADED

const std::string SOUNDS_MAP[_SOUND_MAX] = {
    "none", // SOUND_NONE
    "audio/focus_change.wav", // SOUND_FOCUS_CHANGE
    "audio/focus_error.wav", // SOUND_FOCUS_ERROR
    "audio/click.wav", // SOUND_CLICK
    "audio/back.wav", // SOUND_BACK
    "audio/focus_sidebar.wav", // SOUND_FOCUS_SIDEBAR
    "audio/click_error.wav", // SOUND_CLICK_ERROR
    "none", // SOUND_HONK
    "audio/click_sidebar.wav", // SOUND_CLICK_SIDEBAR
    "audio/touch_unfocus.wav", // SOUND_TOUCH_UNFOCUS
    "audio/click.wav", // SOUND_TOUCH
    "audio/slider_tick.wav", // SOUND_SLIDER_TICK
    "audio/touch_unfocus.wav" // SOUND_SLIDER_RELEASE
};

SDLAudioPlayer::SDLAudioPlayer()
{
    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        Logger::error("Unable to init sdl audio player: {}", SDL_GetError());
        return;
    }

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 44100 };

    audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioStream));
}

bool SDLAudioPlayer::load(enum Sound sound)
{
    if (this->init)
        return false;

    if (sound == SOUND_NONE)
        return true;

    const std::string soundName = SOUNDS_MAP[sound];

    AudioData data {};
    SDL_AudioSpec wavSpec;

    uint8_t* wavBuffer = nullptr;
    uint32_t wavLenght = 0;

#ifdef USE_LIBROMFS
    const auto& soundData = romfs::get(soundName);

    if (!soundData.valid())
    {
        Logger::warning("Unable to load sound {}", soundName);
        return false;
    }

    // Convert raw ROMFS data to an SDL audio buffer
    SDL_IOStream* rw = SDL_IOFromMem(const_cast<void*>(static_cast<const void*>(soundData.data())), soundData.size());
    if (!rw)
    {
        Logger::warning("Failed to create IOStream for sound {}: {}", soundName, SDL_GetError());
        return false;
    }
    if (!SDL_LoadWAV_IO(rw, true, &wavSpec, &wavBuffer, &wavLenght))
    {
        Logger::warning("Failed to load WAV from ROMFS {}: {}", soundName, SDL_GetError());
        // destroy iostream
        return false;
    }
#else
    if (!SDL_LoadWAV(soundName.c_str(), &wavSpec, &wavBuffer, &wavLenght))
    {
        Logger::warning("Failed to load audio: {}", soundName, SDL_GetError());
        return false;
    }
#endif

    data.buffer   = wavBuffer;
    data.lenght   = wavLenght;
    sounds[sound] = data;
    Logger::debug("Succesfuly load sound {}: {}", soundName, wavLenght);
    return true;
}

bool SDLAudioPlayer::play(Sound sound, float pitch)
{
    if (sound == SOUND_NONE)
    {
        return true;
    }

    if (sounds.find(sound) == sounds.end())
    {
        if (!this->load(sound))
            return false;
    }

    SDL_ClearAudioStream(audioStream);
    const AudioData& data = sounds[sound];
    if (!SDL_PutAudioStreamData(audioStream, data.buffer, data.lenght))
    {
        Logger::error("Unable to play sound: {}", SDL_GetError());
        return false;
    }

    return true;
}

SDLAudioPlayer::~SDLAudioPlayer()
{
    if (!this->init)
        return;

    SDL_DestroyAudioStream(audioStream);
}
}