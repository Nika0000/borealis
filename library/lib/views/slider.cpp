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

#include <borealis/core/application.hpp>
#include <borealis/core/touch/pan_gesture.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/core/util.hpp>
#include <borealis/views/slider.hpp>

namespace brls
{

Slider::Slider()
{
    m_line      = new Rectangle();
    m_lineEmpty = new Rectangle();
    m_pointer   = new Rectangle();

    m_line->detach();
    m_lineEmpty->detach();
    m_pointer->detach();

    addView(m_pointer);
    addView(m_line);
    addView(m_lineEmpty);

    setHeight(40);

    m_line->setHeight(7);
    m_line->setCornerRadius(3.5f);

    m_lineEmpty->setHeight(7);
    m_lineEmpty->setCornerRadius(3.5f);

    m_pointer->setHeight(m_basePointerSize);
    m_pointer->setWidth(m_basePointerSize);
    m_pointer->setCornerRadius(m_basePointerSize / 2);
    m_pointer->setHighlightCornerRadius(m_basePointerSize / 2 + 2);
    m_pointer->setBorderThickness(4);
    m_pointer->setShadowType(ShadowType::GENERIC);
    m_pointer->setShadowVisibility(true);
    m_pointer->setFocusable(true);

    Theme theme = Application::getTheme();
    m_pointer->setColor(theme["brls/slider/pointer_color"]);
    m_pointer->setBorderColor(theme["brls/slider/pointer_border_color"]);

    m_line->setColor(theme["brls/slider/line_filled"]);
    m_lineEmpty->setColor(theme["brls/slider/line_empty"]);

    auto move = [this](float dir)
    {
        if (m_step <= 0)
            return;

        float next   = m_progress + dir * m_step;
        bool atBound = next <= 0 || next >= 1;
        next         = next < 0 ? 0 : (next > 1 ? 1 : next);

        if (next == m_progress)
            return;

        if (atBound)
        {
            m_pointer->shakeHighlight(dir > 0 ? FocusDirection::RIGHT : FocusDirection::LEFT);
            Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
        }
        else
            Application::getAudioPlayer()->play(SOUND_SLIDER_TICK);

        setProgress(next);
    };

    m_pointer->registerAction(
        "",
        BUTTON_NAV_RIGHT,
        [move](View*)
        {
            move(+1);
            return true;
        },
        true,
        true,
        SOUND_NONE
    );
    m_pointer->registerAction(
        "",
        BUTTON_NAV_LEFT,
        [move](View*)
        {
            move(-1);
            return true;
        },
        true,
        true,
        SOUND_NONE
    );

    m_pointer->registerAction("", BUTTON_RIGHT, [](View*) { return true; }, true, true, SOUND_NONE);
    m_pointer->registerAction("", BUTTON_LEFT, [](View*) { return true; }, true, true, SOUND_NONE);

    m_pointer->registerAction("", BUTTON_A, [](View* view) { return true; }, true, false, SOUND_NONE);

    this->addGestureRecognizer(new PanGestureRecognizer(
        [this](PanGestureStatus status, Sound* soundToPlay)
        {
            Application::giveFocus(m_pointer);

            if (status.state == GestureState::INTERRUPTED || status.state == GestureState::FAILED)
            {
                setPointerPressed(false);
                *soundToPlay = SOUND_TOUCH_UNFOCUS;
                return;
            }

            if (status.state == GestureState::UNSURE)
                *soundToPlay = SOUND_FOCUS_CHANGE;

            if (status.state == GestureState::START)
                setPointerPressed(true);

            if (status.state != GestureState::START && status.state != GestureState::STAY && status.state != GestureState::END)
                return;

            float paddingWidth = getWidth() - m_basePointerSize;
            float x            = status.position.x - getX() - m_basePointerSize / 2;
            float prev         = m_progress;

            // Pointer follows the finger freely; tick whenever we cross into a new step.
            setProgress(paddingWidth > 0 ? x / paddingWidth : 0);
            if (m_step > 0 && (int) roundf(m_progress / m_step) != (int) roundf(prev / m_step))
                Application::getAudioPlayer()->play(SOUND_SLIDER_TICK);

            if (status.state == GestureState::END)
            {
                setPointerPressed(false);
                // Snap to the nearest tick once the finger is lifted.
                if (m_step > 0)
                    setProgress(roundf(m_progress / m_step) * m_step);
                Application::getPlatform()->getAudioPlayer()->play(SOUND_SLIDER_RELEASE);
            }
        },
        PanAxis::HORIZONTAL
    ));

    this->addGestureRecognizer(new TapGestureRecognizer(
        [this](TapGestureStatus status, Sound* soundToPlay)
        {
            if (status.state != GestureState::END)
                return;

            Application::giveFocus(m_pointer);

            float paddingWidth = getWidth() - m_basePointerSize;
            float x            = status.position.x - getX() - m_basePointerSize / 2;
            setProgress(paddingWidth > 0 ? x / paddingWidth : 0);
            if (m_step > 0)
                setProgress(roundf(m_progress / m_step) * m_step);

            *soundToPlay = SOUND_SLIDER_RELEASE;
        }
    ));

    m_progress = 0.33f;
}

void Slider::onLayout()
{
    Box::onLayout();
    updateUI();
}

void Slider::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    continuousButtonProcessing();
    Box::draw(vg, x, y, width, height, style, ctx);
}

void Slider::continuousButtonProcessing()
{
    // Only continuous sliders glide per-frame; stepped sliders move via registerAction.
    if (m_step > 0 || !m_pointer->isFocused())
    {
        m_holdTime = 0.0f;
        m_boundHit = false;
        return;
    }

    ControllerState state;
    Application::getPlatform()->getInputManager()->updateUnifiedControllerState(&state);

    bool right = state.buttons[BUTTON_NAV_RIGHT];
    bool left  = state.buttons[BUTTON_NAV_LEFT];

    if (right == left) // nothing held, or both (ambiguous): reset
    {
        m_holdTime = 0.0f;
        m_boundHit = false;
        return;
    }

    float dir = right ? 1.0f : -1.0f;
    float fps = Application::getFPS();
    if (fps <= 0)
        fps = 60.0f;

    m_holdTime += 1.0f / fps;

    float speed = 0.01f + std::min(m_holdTime / 2.0f, 1.0f) * 0.79f;
    float next  = m_progress + dir * speed / fps;
    next        = next < 0 ? 0 : (next > 1 ? 1 : next);

    if (next == m_progress) // pinned against a bound
    {
        if (!m_boundHit)
        {
            m_boundHit = true;
            m_pointer->shakeHighlight(dir > 0 ? FocusDirection::RIGHT : FocusDirection::LEFT);
            Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
        }
        return;
    }

    m_boundHit = false;
    setProgress(next);
}

View* Slider::getDefaultFocus() { return m_pointer; }

void Slider::setProgress(float progress)
{
    m_progress = progress < 0 ? 0 : (progress > 1 ? 1 : progress);

    m_progressEvent.fire(m_progress);
    updateUI();
}

void Slider::updateUI()
{
    float pointerSize    = m_basePointerSize * m_pointerScale;
    float paddingWidth   = getWidth() - m_basePointerSize;
    float lineStart      = m_basePointerSize / 2;
    float lineStartWidth = paddingWidth * m_progress;
    float lineEnd        = paddingWidth * m_progress + m_basePointerSize / 2;
    float lineEndWidth   = paddingWidth * (1 - m_progress);
    float lineYPos       = getHeight() / 2 - m_line->getHeight() / 2;

    m_line->setDetachedPosition(lineStart, lineYPos);
    m_line->setWidth(lineStartWidth);

    m_lineEmpty->setDetachedPosition(round(lineEnd), lineYPos);
    m_lineEmpty->setWidth(lineEndWidth);

    m_pointer->setWidth(pointerSize);
    m_pointer->setHeight(pointerSize);
    m_pointer->setCornerRadius(pointerSize / 2);
    m_pointer->setHighlightCornerRadius(pointerSize / 2 + 2);
    m_pointer->setDetachedPosition(lineEnd - pointerSize / 2, getHeight() / 2 - pointerSize / 2);
}

void Slider::setPointerPressed(bool pressed)
{
    m_pointerScale.stop();
    m_pointerScale.reset(m_pointerScale);
    m_pointerScale.addStep(pressed ? 1.30f : 1.0f, 120, EasingFunction::quadraticOut);
    m_pointerScale.setTickCallback([this] { this->pointerScaleTick(); });
    m_pointerScale.start();
}

void Slider::pointerScaleTick() { updateUI(); }

float Slider::getProgress() const { return m_progress; }

Event<float>* Slider::getProgressEvent() { return &m_progressEvent; }

void Slider::setStep(float step) { m_step = step; }

void Slider::setPointerSize(float size)
{
    this->m_basePointerSize = size;
    this->updateUI();
}

View* Slider::create() { return new Slider(); }

} // namespace brls
