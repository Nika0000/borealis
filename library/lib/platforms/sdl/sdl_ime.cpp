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

#include <borealis/core/application.hpp>
#include <borealis/core/event.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_ime.hpp>
#include <borealis/platforms/sdl/sdl_video.hpp>
#include <borealis/views/edit_text_dialog.hpp>

#include "borealis/core/thread.hpp"

#ifdef __PSV__
extern "C" uint8_t* vita_ime_init_text;
__attribute__((weak)) uint8_t* vita_ime_init_text = nullptr;

extern "C" uint32_t vita_ime_max_text;
__attribute__((weak)) uint32_t vita_ime_max_text = 32;

extern "C" uint32_t vita_ime_type;
__attribute__((weak)) uint32_t vita_ime_type = 0;
#endif

namespace brls
{
SDLImeManager::SDLImeManager(Event<SDL_Event*>* event) : m_event(event), m_cursor(-1) {}

static int utf8_len(std::string& s)
{
    int result = 0;
    for (auto& it : s)
    {
        if ((it & 0xc0) != 0x80)
        {
            result += 1;
        }
    }
    return result;
}

static int utf8_find_prev(std::string& s, int size)
{
    int result = 0;
    for (int i = s.size() - 1; i >= 0; i--)
    {
        char p = s.at(i);
        result += 1;
        if ((p & 0xc0) != 0x80)
        {
            size--;
        }
        if (size <= 0)
        {
            break;
        }
    }
    return result;
}

static int utf8_find_next(std::string& s, int offset, int size)
{
    int result = 0;
    if (size == 0)
    {
        return 0;
    }
    for (size_t i = offset; i < s.size(); i++)
    {
        char p = s.at(i);
        if ((p & 0xc0) != 0x80)
        {
            size--;
        }
        if (size < 0)
        {
            break;
        }
        result += 1;
    }
    return result;
}

void SDLImeManager::openInputDialog(
    const std::function<void(std::string)>& cb,
    const std::string& headerText,
    const std::string& subText,
    size_t maxStringLength,
    const std::string& initialText
)
{
    SDLVideoContext* videoContext = (SDLVideoContext*) Application::getPlatform()->getVideoContext();
    SDL_Window* window            = videoContext->getSDLWindow();
    EditTextDialog* dialog        = new EditTextDialog();
    m_inputBuffer                 = initialText;
#ifdef __PSV__
    vita_ime_init_text = reinterpret_cast<uint8_t*>(const_cast<char*>(m_inputBuffer.c_str()));
    vita_ime_max_text  = maxStringLength;
#endif
    auto updateText = [this, dialog, maxStringLength]()
    {
        std::string text = m_inputBuffer;
        if (this->m_editingBuffer.size() > 0)
        {
            std::string editing = fmt::format("[{}]", this->m_editingBuffer);
            if (m_cursor >= 0)
            {
                int start = utf8_find_next(m_inputBuffer, 0, m_cursor);
                text.insert(start, editing);
            }
            else
            {
                text += editing;
            }
        }
        dialog->setText(text);
        dialog->setCountText(fmt::format("{}/{}", utf8_len(m_inputBuffer), maxStringLength));

        if (!Application::isInputBlocked())
        {
            Application::blockInputs(InputType::GAMEPAD, true);
            static size_t iter = 0;
            cancelDelay(iter);
            iter = delay(500, []() { Application::unblockInputs(); });
        }
    };
    auto updateTextCursor = [this, dialog]()
    {
        int cursor = this->m_cursor;
        if (cursor >= (int) CursorPosition::START && this->m_editingBuffer.size() > 0)
        {
            cursor += utf8_len(this->m_editingBuffer) + 2;
        }
        dialog->setCursor(cursor);
    };
    auto updateTextAndCursor = [this, updateText, updateTextCursor, maxStringLength](std::string text)
    {
        size_t prev_n = utf8_len(m_inputBuffer);
        if (prev_n >= maxStringLength)
        {
            return;
        }
        int n = utf8_len(text);
        if (prev_n + n > maxStringLength)
        {
            n       = maxStringLength - prev_n;
            int end = utf8_find_next(text, 0, n);
            text    = text.substr(0, end);
        }
        if (this->m_cursor >= 0)
        {
            int start = utf8_find_next(m_inputBuffer, 0, this->m_cursor);
            m_inputBuffer.insert(start, text);
            this->m_cursor += n;
        }
        else
        {
            m_inputBuffer += text;
        }
        updateTextCursor();
        updateText();
    };
    dialog->setHeaderText(headerText);
    dialog->setHintText(subText);
    m_cursor = -1;
    if (!initialText.empty())
    {
        updateText();
        updateTextCursor();
    }
#if defined(BOREALIS_USE_D3D11) || defined(BOREALIS_USE_METAL)
    float scale = Application::windowScale;
#else
    float scale = Application::windowScale / Application::getPlatform()->getVideoContext()->getScaleFactor();
#endif
    // 更新输入法条位置
    dialog->getLayoutEvent()->subscribe(
        [window, scale](Point p)
        {
#ifndef PS4
            const SDL_Rect rect = { (int) (p.x * scale), (int) (p.y * scale), 100, 20 };
            SDL_SetTextInputArea(window, &rect, 0);
#endif
        }
    );

    dialog->getClipboardEvent()->subscribe(
        [this, updateTextAndCursor](const std::string& str)
        {
            if (this->m_isEditing || str.empty())
                return;
            updateTextAndCursor(str);
        }
    );

    auto eventID1 = m_event->subscribe(
        [this, updateTextAndCursor, updateText, updateTextCursor](SDL_Event* e)
        {
            switch (e->type)
            {
                case SDL_EVENT_TEXT_INPUT:
                    this->m_isEditing = false;
                    updateTextAndCursor(e->text.text);
                    break;
                case SDL_EVENT_TEXT_EDITING:
                    this->m_isEditing     = strlen(e->edit.text) != 0;
                    this->m_editingBuffer = e->edit.text;
                    updateTextCursor();
                    updateText();
                    break;
            }
        }
    );

    dialog->registerAction(
        "hints/left"_i18n,
        BUTTON_LEFT,
        [this, updateTextCursor](...)
        {
            if (this->m_isEditing)
                return true;
            if (this->m_cursor == (int) CursorPosition::END)
            {
                this->m_cursor = utf8_len(m_inputBuffer) - 1;
                if (this->m_cursor < 0)
                    this->m_cursor = 0;
                updateTextCursor();
            }
            else if (this->m_cursor > (int) CursorPosition::START)
            {
                this->m_cursor--;
                updateTextCursor();
            }
            return true;
        },
        true,
        true
    );
    dialog->registerAction(
        "hints/right"_i18n,
        BUTTON_RIGHT,
        [this, updateTextCursor](...)
        {
            if (this->m_isEditing)
                return true;
            if (this->m_cursor >= (int) CursorPosition::START)
            {
                if (this->m_cursor < utf8_len(m_inputBuffer))
                {
                    this->m_cursor++;
                    updateTextCursor();
                }
            }
            return true;
        },
        true,
        true
    );

    // delete text
    dialog->getBackspaceEvent()->subscribe(
        [this, updateText, updateTextCursor](...)
        {
            if (m_inputBuffer.empty())
                return true;
            if (this->m_cursor == (int) CursorPosition::START)
                return true;
            if (this->m_cursor > (int) CursorPosition::START)
            {
                int start = utf8_find_next(m_inputBuffer, 0, this->m_cursor - 1);
                int n     = utf8_find_next(m_inputBuffer, start, 1);
                m_inputBuffer.erase(start, n);
                this->m_cursor -= 1;
                updateTextCursor();
            }
            else
            {
                int offset = utf8_find_prev(m_inputBuffer, 1);
                m_inputBuffer.erase(m_inputBuffer.size() - offset, offset);
            }
            updateText();
            return true;
        }
    );

    // cancel
    dialog->getCancelEvent()->subscribe(
        [this, window, eventID1]()
        {
            SDL_StopTextInput(window);
            this->m_event->unsubscribe(eventID1);
            Application::getPlatform()->getInputManager()->clearInputState();
        }
    );

    // submit
    dialog->getSubmitEvent()->subscribe(
        [this, window, eventID1, cb]()
        {
            SDL_StopTextInput(window);
            this->m_event->unsubscribe(eventID1);
            Application::getPlatform()->getInputManager()->clearInputState();
            cb(this->m_inputBuffer);
            return true;
        }
    );

    Application::getPlatform()->getInputManager()->clearInputState();
    SDL_StartTextInput(window);
    dialog->open();
}

bool SDLImeManager::openForText(
    std::function<void(std::string)> f,
    std::string headerText,
    std::string subText,
    int maxStringLength,
    std::string initialText,
    int kbdDisableBitmask
)
{
#ifdef __PSV__
    vita_ime_type = 0;
#endif
    this->openInputDialog([f](const std::string& text) { f(text); }, headerText, subText, maxStringLength, initialText);
    return true;
}

bool SDLImeManager::openForNumber(
    std::function<void(long)> f,
    std::string headerText,
    std::string subText,
    int maxStringLength,
    std::string initialText,
    std::string leftButton,
    std::string rightButton,
    int kbdDisableBitmask
)
{
#ifdef __PSV__
    vita_ime_type = 2;
#endif
    this->openInputDialog(
        [f](const std::string& text)
        {
            if (text.empty())
                return;
            try
            {
                f(stoll(text));
            }
            catch (const std::invalid_argument& e)
            {
                Logger::error("Could not parse input, did you enter a valid integer? {}", e.what());
            }
            catch (const std::out_of_range& e)
            {
                Logger::error("Out of range: {}", e.what());
            }
            catch (const std::exception& e)
            {
                Logger::error("Unexpected error occurred: {}", e.what());
            }
        },
        headerText,
        subText,
        maxStringLength,
        initialText
    );
    return true;
}
}