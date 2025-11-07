/*
    Copyright 2025 Nika0000

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

#include <borealis.hpp>
#include <borealis/core/touch/hover_gesture.hpp>

namespace brls
{

HoverGestureRecognizer::HoverGestureRecognizer(View* view, HoverGestureConfig config)
{
    hoverEvent.subscribe([view, config](HoverGestureStatus status)
        {
            if (status.state == GestureState::START)
            {
                Application::getPlatform()->getInputManager()->setCursorType(config.cursor);
                view->setClickAlpha(0.8f);
            }

            if (status.state == GestureState::END)
            {
                Application::getPlatform()->getInputManager()->setCursorType(CursorType::CURSOR_DEFAULT);
                view->setClickAlpha(0.0f);
            } });
}

HoverGestureRecognizer::HoverGestureRecognizer(HoverGestureEvent::Callback respond)
{
    hoverEvent.subscribe(respond);
}

HoverGestureRecognizer::~HoverGestureRecognizer()
{
    if (lastState != GestureState::END)
        Application::getPlatform()->getInputManager()->setCursorType(CursorType::CURSOR_DEFAULT);
}

GestureState HoverGestureRecognizer::recognitionLoop(TouchState touch, MouseState mouse, View*, Sound*)
{
    if (!enabled)
        return GestureState::FAILED;

    // Hover is mouse-only: ignore touch-based interactions
    if (touch.phase != TouchPhase::NONE)
    {
        this->state = GestureState::FAILED;
        lastState   = this->state;
        return this->state;
    }

    // Synthetic leave signal from Application
    if (mouse.middleButton == TouchPhase::END)
    {
        this->state = GestureState::END;
        if (lastState != this->state)
            this->hoverEvent.fire(getCurrentStatus());
        lastState = this->state;
        return this->state;
    }

    // If any mouse button is engaged, not a hover
    bool anyButton = mouse.leftButton != TouchPhase::NONE || mouse.middleButton != TouchPhase::NONE || mouse.rightButton != TouchPhase::NONE;
    if (anyButton)
    {
        this->state = GestureState::FAILED;
        lastState   = this->state;
        return this->state;
    }

    // Track position and delta
    this->position = mouse.position;

    if (this->state != GestureState::START && this->state != GestureState::STAY)
    {
        // First frame over the view
        this->delta        = Point();
        this->lastPosition = this->position;
        this->state        = GestureState::START;
    }
    else
    {
        this->delta        = this->lastPosition - this->position;
        this->lastPosition = this->position;
        this->state        = GestureState::STAY;
    }

    // Fire event each active frame
    this->hoverEvent.fire(getCurrentStatus());

    lastState = this->state;
    return this->state;
}

HoverGestureStatus HoverGestureRecognizer::getCurrentStatus() const
{
    return HoverGestureStatus {
        .state    = this->state,
        .position = this->position,
        .delta    = this->delta,
    };
}

} // namespace brls