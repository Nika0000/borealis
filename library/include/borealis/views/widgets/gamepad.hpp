#pragma once

#include <array>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/core/input.hpp>
#include <borealis/views/image.hpp>

namespace brls
{

class GamepadWidget : public Box
{
  public:
    GamepadWidget();
    ~GamepadWidget() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    static View* create();

  private:
    static constexpr int MAX_CONTROLLERS = 4;

    int m_controllerCount = 0;
    Event<ControllerInfo>::Subscription m_connectionSub;
    std::array<Image*, MAX_CONTROLLERS> m_icons {};

    void refreshCount();
};

} // namespace brls
