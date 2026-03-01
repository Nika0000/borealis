/*
    Copyright 2024 xfangfang

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

#include <borealis/views/widgets/gamepad.hpp>

namespace brls
{

GamepadWidget::GamepadWidget()
{
    // Seed from actual connected count so we're correct on startup
    refreshCount();

    InputManager* inputManager = Application::getPlatform()->getInputManager();

    connectionSub = inputManager->getControllerConnectionEvent()->subscribe([this](const ControllerInfo& e)
        {
            if (e.state == ControllerConnectionState::CONNECTED)
                controllerCount++;
            else if (e.state == ControllerConnectionState::DISCONNECTED)
                controllerCount = std::max(0, controllerCount - 1);

            refreshCount();

            //
        });
}

GamepadWidget::~GamepadWidget()
{
    Application::getPlatform()
        ->getInputManager()
        ->getControllerConnectionEvent()
        ->unsubscribe(connectionSub);
}

void GamepadWidget::refreshCount()
{
    controllerCount = Application::getPlatform()->getInputManager()->getControllersConnectedCount();

    float totalWidth = controllerCount > 0
        ? ICON_SIZE * controllerCount + ICON_SPACING * (controllerCount - 1)
        : 0.0f;
    setWidth(totalWidth);
    setHeight(controllerCount > 0 ? ICON_SIZE : 0.0f);
    setVisibility(controllerCount > 0 ? Visibility::VISIBLE : Visibility::GONE);
}

void GamepadWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    if (controllerCount <= 0)
        return;

    nvgSave(vg);
    nvgFontFaceId(vg, Application::getFont(FONT_MATERIAL_ICONS));
    nvgFontSize(vg, ICON_SIZE);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    float cx = x;
    for (int i = 0; i < controllerCount; i++)
    {
        const auto& c = PLAYER_COLORS[i % PLAYER_COLORS.size()];
        nvgFillColor(vg, nvgRGB(c[0], c[1], c[2]));
        nvgText(vg, cx, y + height * 0.5f, GAMEPAD_ICON, nullptr);
        cx += ICON_SIZE + ICON_SPACING;
    }

    nvgRestore(vg);

    Box::draw(vg, x, y, width, height, style, ctx);
}

View* GamepadWidget::create()
{
    return new GamepadWidget();
}

} // namespace brls
