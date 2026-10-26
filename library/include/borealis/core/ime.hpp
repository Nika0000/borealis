/*
    Copyright 2019 WerWolv
    Copyright 2019 p-sam
    Copyright 2023 xfangfang

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

#include <borealis/core/geometry.hpp>
#include <functional>
#include <string>

namespace brls
{

enum KeyboardKeyDisableBitmask
{
    KEYBOARD_DISABLE_NONE         = 0,
    KEYBOARD_DISABLE_SPACE        = 1,
    KEYBOARD_DISABLE_AT           = 1 << 1,
    KEYBOARD_DISABLE_PERCENT      = 1 << 2,
    KEYBOARD_DISABLE_FORWSLASH    = 1 << 3,
    KEYBOARD_DISABLE_BACKSLASH    = 1 << 4,
    KEYBOARD_DISABLE_NUMBERS      = 1 << 5,
    KEYBOARD_DISABLE_DOWNLOADCODE = 1 << 6,
    KEYBOARD_DISABLE_USERNAME     = 1 << 7,
};

enum class InlineTextKey
{
    BACKSPACE,
    DELETE_FORWARD,
    LEFT,
    RIGHT,
    HOME,
    END,
    UP,
    DOWN,
};

/**
 * Non-modal text input session. The owning view keeps the text; the backend
 * only reports input. All callbacks run on the UI thread and may be empty.
 */
struct InlineTextConfig
{
    std::string initialText; // advisory: the view owns the text
    int maxLength         = 0; // in code points, 0 = unlimited. Advisory: the view enforces it
    int kbdDisableBitmask = KEYBOARD_DISABLE_NONE; // single-character bits filter committed/pasted text

    // Caret rectangle in view (content) coordinates, used to place the IME candidate window
    std::function<Rect()> caretRect;

    std::function<void(const std::string&)> onCommit; // finished text (UTF-8)
    std::function<void(const std::string&, int)> onPreedit; // composition (UTF-8) + caret in code points; empty = cleared
    std::function<void(InlineTextKey)> onKey; // backspace / delete / cursor movement
    std::function<void(const std::string&)> onPaste; // clipboard text (UTF-8, control characters stripped, bitmask applied)

    // Fired once whenever the session ends, including when the backend closes it
    // (e.g. an EditTextDialog opens) and when endInlineText() is called by the owner.
    std::function<void()> onEnd;
};

/** Removes control characters (newlines, tabs, < 0x20, 0x7f), used for pasted text. */
inline std::string stripInlineControl(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (char c : text)
    {
        unsigned char u = (unsigned char)c;
        if (u < 0x20 || u == 0x7f)
            continue;
        out.push_back(c);
    }
    return out;
}

/**
 * Removes characters disallowed by a KeyboardKeyDisableBitmask. Only the bits
 * that map to single characters are applied; USERNAME/DOWNLOADCODE are left to the caller.
 */
inline std::string filterInlineText(const std::string& text, int mask)
{
    if (mask == KEYBOARD_DISABLE_NONE)
        return text;
    std::string out;
    out.reserve(text.size());
    for (char c : text)
    {
        if ((mask & KEYBOARD_DISABLE_SPACE) && c == ' ')
            continue;
        if ((mask & KEYBOARD_DISABLE_AT) && c == '@')
            continue;
        if ((mask & KEYBOARD_DISABLE_PERCENT) && c == '%')
            continue;
        if ((mask & KEYBOARD_DISABLE_FORWSLASH) && c == '/')
            continue;
        if ((mask & KEYBOARD_DISABLE_BACKSLASH) && c == '\\')
            continue;
        if ((mask & KEYBOARD_DISABLE_NUMBERS) && c >= '0' && c <= '9')
            continue;
        out.push_back(c);
    }
    return out;
}

class ImeManager
{
  public:
    virtual ~ImeManager() { }

    virtual bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
        = 0;

    virtual bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE)
        = 0;

    /**
     * True if beginInlineText() is available (desktop GLFW/SDL only).
     * Callers must fall back to openForText() otherwise.
     */
    virtual bool supportsInline() const { return false; }

    /**
     * Starts a non-modal text session. Only one can be active; fails if one is
     * already active or an EditTextDialog session is open.
     */
    virtual bool beginInlineText(const InlineTextConfig& config) { return false; }

    /** Safe to call from inside any session callback. Fires InlineTextConfig::onEnd. */
    virtual void endInlineText() { }

    virtual bool isInlineTextActive() const { return false; }

    /** Re-reads InlineTextConfig::caretRect and moves the candidate window. */
    virtual void updateInlineCaret() { }
};

};