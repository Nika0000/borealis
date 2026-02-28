/*
Copyright 2025 Nika0000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

#include <SDL3/SDL_audio.h>

#include <borealis/core/audio.hpp>
#include <cstdint>
#include <unordered_map>

namespace brls
{

struct AudioData
{
    uint8_t* buf;
    uint32_t len;
};

class SDLAudioPlayer : public AudioPlayer
{
  public:
    SDLAudioPlayer();
    ~SDLAudioPlayer();

    bool load(enum Sound sound) override;
    bool play(enum Sound sound, float pitch) override;

    bool loadAudioFromFile(const std::string& name, const std::string& filePath) override;
    bool loadAudioFromResource(const std::string& name, const std::string& resourcePath) override;
    bool play(const std::string& name, float pitch, bool foreground) override;
    void unloadUserAudio(const std::string& name) override;

    [[nodiscard]] SDL_AudioDeviceID getDevice() const { return audioDevice; }

  private:
    bool playAudioData(const AudioData& data, float pitch, SDL_AudioStream* stream);

    bool init = false;
    std::unordered_map<Sound, AudioData> sounds;
    std::unordered_map<std::string, AudioData> userSounds;

    SDL_AudioStream* audioStream; // main UI/system stream
    SDL_AudioStream* userAudioStream; // independent user-audio stream (foreground)
    SDL_AudioDeviceID audioDevice;
};

}