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
    setGap(4);

    for (int i = 0; i < MAX_CONTROLLERS; i++)
    {
        m_icons[i] = new Image();
        m_icons[i]->setSize(Size(44, 44));
        m_icons[i]->setScalingType(ImageScalingType::FIT);
        m_icons[i]->setImageFromRes("img/sys/controller_p" + std::to_string(i + 1) + ".png");
        m_icons[i]->setVisibility(Visibility::GONE);
        addView(m_icons[i]);
    }

    refreshCount();

    InputManager* inputManager = Application::getPlatform()->getInputManager();

    m_connectionSub = inputManager->getControllerConnectionEvent()->subscribe(
        [this](const ControllerInfo& e)
        {
            if (e.state == ControllerConnectionState::CONNECTED)
                m_controllerCount = std::min(MAX_CONTROLLERS, m_controllerCount + 1);
            else if (e.state == ControllerConnectionState::DISCONNECTED)
                m_controllerCount = std::max(0, m_controllerCount - 1);

            refreshCount();
        }
    );
}

GamepadWidget::~GamepadWidget()
{
    Application::getPlatform()->getInputManager()->getControllerConnectionEvent()->unsubscribe(m_connectionSub);
}

void GamepadWidget::refreshCount()
{
    int count         = Application::getPlatform()->getInputManager()->getControllersConnectedCount();
    m_controllerCount = std::min(MAX_CONTROLLERS, count);

    for (int i = 0; i < MAX_CONTROLLERS; i++)
        m_icons[i]->setVisibility(i < m_controllerCount ? Visibility::VISIBLE : Visibility::GONE);

    setVisibility(m_controllerCount > 0 ? Visibility::VISIBLE : Visibility::GONE);
}

void GamepadWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    Box::draw(vg, x, y, width, height, style, ctx);
}

View* GamepadWidget::create() { return new GamepadWidget(); }

} // namespace brls
