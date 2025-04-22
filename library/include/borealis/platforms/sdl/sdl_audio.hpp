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
class SDLAudioPlayer : public AudioPlayer
{
  public:
    SDLAudioPlayer();
    ~SDLAudioPlayer();

    struct AudioData
    {
        uint8_t* buffer;
        uint32_t lenght;
        uint32_t position;
    };

    bool load(enum Sound sound) override;
    bool play(enum Sound sound, float pitch) override;

  private:
    bool init = false;
    std::unordered_map<Sound, AudioData> sounds;
    SDL_AudioStream* audioStream;
};

}