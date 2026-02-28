/*
    Copyright 2021 natinusala

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
#include <string>

namespace brls
{

enum Sound
{
    SOUND_NONE = 0, // no sound

    SOUND_FOCUS_CHANGE, // played when the focus changes
    SOUND_FOCUS_ERROR, // played when the user wants to go somewhere impossible (while the highlight wiggles)
    SOUND_CLICK, // played when the click action runs
    SOUND_BACK, // played when back action runs
    SOUND_FOCUS_SIDEBAR, // played when the focus changes to a sidebar item
    SOUND_CLICK_ERROR, // played when the user clicks a disabled button / a view focused with no click action
    SOUND_HONK, // honk
    SOUND_CLICK_SIDEBAR, // played when a sidebar item is clicked
    SOUND_TOUCH_UNFOCUS, // played when touch focus has been interrupted
    SOUND_TOUCH, // played when touch doesn't require it's own click sound
    SOUND_SLIDER_TICK,
    SOUND_SLIDER_RELEASE,

    _SOUND_MAX, // not an actual sound, just used to count of many sounds there are
};

// Platform agnostic Audio player
// Each platform's AudioPlayer is responsible for managing the enum Sound -> internal representation map
class AudioPlayer
{
  public:
    virtual ~AudioPlayer() { };

    /**
     * Preemptively loads the given sound so that it's ready to be played
     * when needed.
     *
     * Returns a boolean indicating if the sound has been loaded or not.
     */
    virtual bool load(enum Sound sound) = 0;

    /**
     * Plays the given sound.
     *
     * The AudioPlayer should not assume that the sound has been
     * loaded already, and must load it if needed.
     *
     * Returns a boolean indicating if the sound has been played or not.
     */
    virtual bool play(enum Sound sound, float pitch = 1) = 0;

    /**
     * Loads a user audio WAV sound from a file path and registers it under
     * the given name.  Calling this again with the same name replaces the
     * previously loaded sound.
     *
     * Returns true on success.
     */
    virtual bool loadAudioFromFile(const std::string& name, const std::string& filePath) = 0;

    /**
     * Loads a user audio WAV sound from a resource path and registers it
     * under the given name.
     *
     * When USE_LIBROMFS is defined the file is fetched from the embedded
     * ROMFS archive using the given path (e.g. "audio/notify.wav").
     * Otherwise the path is resolved relative to BRLS_RESOURCES on disk,
     * matching the same pattern used by the font loader.
     *
     * Returns true on success.
     */
    virtual bool loadAudioFromResource(const std::string& name, const std::string& resourcePath) = 0;

    /**
     * Plays a previously loaded user audio sound by name.
     *
     * When `foreground` is false (default) the sound is played on the main
     * audio stream: any currently playing UI sound on that stream is stopped
     * first.
     *
     * When `foreground` is true the sound is played on a dedicated independent
     * stream so it mixes alongside UI sounds without interrupting them.
     *
     * Returns true on success.
     */
    virtual bool play(const std::string& name, float pitch = 1, bool foreground = false) = 0;

    /**
     * Releases a previously loaded user audio sound.
     * Does nothing if the name is unknown.
     */
    virtual void unloadUserAudio(const std::string& name) = 0;
};

// An AudioPlayer that does nothing
class NullAudioPlayer : public AudioPlayer
{
  public:
    bool load(enum Sound sound) override
    {
        return false;
    }

    bool play(enum Sound sound, float pitch) override
    {
        return false;
    }

    bool loadAudioFromFile(const std::string&, const std::string&) override
    {
        return false;
    }

    bool loadAudioFromResource(const std::string& /*name*/, const std::string& /*resourcePath*/) override
    {
        return false;
    }

    bool play(const std::string&, float, bool) override
    {
        return false;
    }

    void unloadUserAudio(const std::string&) override { }
};

} // namespace brls
