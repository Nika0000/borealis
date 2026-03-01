#pragma once

#include <nanovg.h>

#include <array>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/core/font.hpp>
#include <borealis/core/input.hpp>

namespace brls
{

// Widget that displays one icon per connected gamepad.
// Hides itself automatically when no controllers are connected.
class GamepadWidget : public Box
{
  public:
    GamepadWidget();
    ~GamepadWidget() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    static View* create();

  private:
    int controllerCount = 0;
    Event<ControllerInfo>::Subscription connectionSub;

    static constexpr float ICON_SIZE    = 28.0f;
    static constexpr float ICON_SPACING = 4.0f;

    static constexpr const char* GAMEPAD_ICON = "\uea28"; // Material gamepad icon

    // Colors cycled per controller index (player 1-4 palette)
    static constexpr std::array<std::array<uint8_t, 3>, 4> PLAYER_COLORS = { {
        { 100, 210, 255 }, // P1 — blue
        { 255, 100, 100 }, // P2 — red
        { 100, 220, 100 }, // P3 — green
        { 255, 210, 80 }, // P4 — yellow
    } };

    void refreshCount();
};

} // namespace brls
