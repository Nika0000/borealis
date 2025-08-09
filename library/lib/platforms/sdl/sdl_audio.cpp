#include "SDL3/SDL_audio.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>

#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_audio.hpp>
#include <string>

#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#endif

namespace brls
{
#define SOUND_LOAD_UNLOADED

const std::string SOUNDS_MAP[_SOUND_MAX] = {
    "none", // SOUND_NONE
    "audio/sys/focus_change.wav", // SOUND_FOCUS_CHANGE
    "audio/sys/focus_error.wav", // SOUND_FOCUS_ERROR
    "audio/sys/click.wav", // SOUND_CLICK
    "audio/sys/back.wav", // SOUND_BACK
    "audio/sys/focus_sidebar.wav", // SOUND_FOCUS_SIDEBAR
    "audio/sys/click_error.wav", // SOUND_CLICK_ERROR
    "none", // SOUND_HONK
    "audio/sys/click_sidebar.wav", // SOUND_CLICK_SIDEBAR
    "audio/sys/touch_unfocus.wav", // SOUND_TOUCH_UNFOCUS
    "audio/sys/touch.wav", // SOUND_TOUCH
    "audio/sys/slider_tick.wav", // SOUND_SLIDER_TICK
    "audio/sys/touch_unfocus.wav" // SOUND_SLIDER_RELEASE
};

SDLAudioPlayer::SDLAudioPlayer()
{
    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        Logger::error("Unable to init sdl audio player: {}", SDL_GetError());
        return;
    }

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 44100 };

    // Main (UI/system) audio stream
    audioStream = SDL_CreateAudioStream(&spec, nullptr);
    if (!audioStream)
    {
        Logger::error("Failed to create main audio stream: {}", SDL_GetError());
        return;
    }

    audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!audioDevice)
    {
        Logger::error("Failed to open audio device: {}", SDL_GetError());
        return;
    }

    // Bind both streams to the same output device
    if (!SDL_BindAudioStream(audioDevice, audioStream) || !SDL_ResumeAudioStreamDevice(audioStream))
    {
        Logger::error("Failed to initialize AudioPlayer: {}", SDL_GetError());
        return;
    }

    init = true;
}

bool SDLAudioPlayer::load(enum Sound sound)
{
    if (!init)
        return false;

    if (sound == SOUND_NONE)
        return true;

    const std::string soundName = SOUNDS_MAP[sound];
    AudioData data {};
    SDL_AudioSpec wavSpec;
    uint8_t* wavBuffer = nullptr;
    uint32_t wavLength = 0;

#ifdef USE_LIBROMFS
    const auto& soundData = romfs::get(soundName);

    if (!soundData.valid())
    {
        Logger::warning("Unable to load sound {}", soundName);
        return false;
    }

    SDL_IOStream* rw = SDL_IOFromMem(const_cast<void*>(static_cast<const void*>(soundData.data())), soundData.size());
    if (!rw)
    {
        Logger::warning("Failed to create IOStream for sound {}: {}", soundName, SDL_GetError());
        return false;
    }
    if (!SDL_LoadWAV_IO(rw, true, &wavSpec, &wavBuffer, &wavLength))
    {
        Logger::warning("Failed to load WAV from ROMFS {}: {}", soundName, SDL_GetError());
        return false;
    }
#else
    std::string soundPath = std::string(BRLS_RESOURCES) + soundName;
    if (!SDL_LoadWAV(soundPath.c_str(), &wavSpec, &wavBuffer, &wavLength))
    {
        Logger::warning("Failed to load audio: {} {}", soundName, SDL_GetError());
        return false;
    }
#endif

    data.buf      = wavBuffer;
    data.len      = wavLength;
    sounds[sound] = data;
    Logger::debug("Successfully loaded sound {}.", soundName);
    return true;
}

bool SDLAudioPlayer::play(Sound sound, float pitch)
{
    if (sound == SOUND_NONE)
        return true;

    if (!sounds.contains(sound))
    {
        if (!this->load(sound))
            return false;
    }

    const AudioData& data = sounds[sound];
    SDL_ClearAudioStream(audioStream);
    SDL_SetAudioStreamFrequencyRatio(audioStream, pitch);

    if (!SDL_PutAudioStreamData(audioStream, data.buf, data.len))
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

    SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyAudioStream(audioStream);
}
}