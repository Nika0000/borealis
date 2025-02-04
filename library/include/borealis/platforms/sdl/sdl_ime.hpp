/*
Copyright 2023 zeromake

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

#include <borealis/core/ime.hpp>
#include <borealis/core/event.hpp>
#include <SDL3/SDL.h>

namespace brls
{
class SDLImeManager : public ImeManager
{
  public:
    SDLImeManager(Event<SDL_Event*> *event);

    bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    void openInputDialog(std::function<void(std::string)> cb, std::string headerText,
        std::string subText, size_t maxStringLength = 50, std::string initialText = "");
  private:
    Event<SDL_Event*> *event;
    int cursor;
    std::string inputBuffer;
    bool isEditing = false; // 是否正在编辑文字
    std::string editingBuffer;
};
}