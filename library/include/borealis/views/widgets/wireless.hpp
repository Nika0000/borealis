/*
    Copyright 2021 XITRIX

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

#include <array>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/image.hpp>

namespace brls
{

class WirelessWidget : public Box
{
  public:
    WirelessWidget();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    static View* create();

  private:
    static constexpr int WIFI_LEVELS = 4;

    std::array<Image*, WIFI_LEVELS> m_wifi {};
    Image* m_ethernet = nullptr;
    Platform* m_platform = nullptr;

    void applyTheme(ThemeVariant theme);
    void updateState();

    inline static bool hasWirelessConnection = false;
    inline static int wifiLevel              = 3;
    inline static bool hasEthernetConnection = false;
};

} // namespace brls
