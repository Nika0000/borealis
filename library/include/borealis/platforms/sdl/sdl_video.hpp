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

#include <SDL3/SDL.h>

#include <borealis/core/video.hpp>
#include <string>

namespace brls
{

/** SDL Video Context providing OpenGL-based rendering via NanoVG. */
class SDLVideoContext : public VideoContext
{
  public:
    /**
     * Creates an SDL window with an OpenGL context and initializes NanoVG.
     * @param title window title
     * @param windowWidth initial window width in pixels
     * @param windowHeight initial window height in pixels
     * @param windowXPos initial horizontal position (SDL_WINDOWPOS_* or coordinate)
     * @param windowYPos initial vertical position (SDL_WINDOWPOS_* or coordinate)
     */
    SDLVideoContext(const std::string& title, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos);
    ~SDLVideoContext() override;

    NVGcontext* getNVGContext() override;
    void clear(NVGcolor color) override;
    void beginFrame() override;
    void endFrame() override;
    void resetState() override;
    void fullScreen(bool fs) override;
    void setSwapInterval(int interval) override;
    double getScaleFactor() override;

    /** Returns the underlying SDL window handle. */
    SDL_Window* getSDLWindow();

    /**
     * Per-frame occlusion poll. Generic entry point; currently only does work
     * on the D3D11 backend, where flip-model swap chains can't report occlusion
     * on their own. No-op on other backends.
     */
    void hitOcclusion();

  private:
    SDL_Window* m_window     = nullptr;
    NVGcontext* m_nvgContext = nullptr;
};

} // namespace brls
