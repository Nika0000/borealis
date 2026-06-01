/*
    Copyright 2020-2021 natinusala
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
#include <borealis/core/touch/scroll_gesture.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/scrolling_frame.hpp>

namespace brls
{

#define SCROLLING_INDICATOR_SIZE 4.0f

BaseScrollingFrame::BaseScrollingFrame(ScrollingAxis axis) : scrollAxis(axis)
{
    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "scrollingBehavior",
        ScrollingBehavior,
        setScrollingBehavior,
        {
            { "natural", ScrollingBehavior::NATURAL },
            { "centered", ScrollingBehavior::CENTERED },
        }
    );

    registerBoolXMLAttribute("showIndicator", [this](bool value) { this->setScrollingIndicatorVisible(value); });

    setupScrollingIndicator();

    input = Application::getPlatform()->getInputManager();
    setFocusable(true);
    setMaximumAllowedXMLElements(1);

    PanAxis panAxis = (scrollAxis == ScrollingAxis::VERTICAL) ? PanAxis::VERTICAL : PanAxis::HORIZONTAL;

    addGestureRecognizer(new ScrollGestureRecognizer(
        [this](PanGestureStatus state, Sound* soundToPlay)
        {
            bool isVertical  = this->scrollAxis == ScrollingAxis::VERTICAL;
            float stateDelta = isVertical ? state.delta.y : state.delta.x;
            float statePos   = isVertical ? state.position.y : state.position.x;
            float stateStart = isVertical ? state.startPosition.y : state.startPosition.x;
            float accelTime  = isVertical ? state.acceleration.time.y : state.acceleration.time.x;
            float accelDist  = isVertical ? state.acceleration.distance.y : state.acceleration.distance.x;

            if (state.state == GestureState::FAILED || state.state == GestureState::UNSURE || state.state == GestureState::INTERRUPTED)
            {
                if (this->isRubberBanding)
                {
                    this->contentOffset.setEndCallback([](bool) {});
                    this->isRubberBanding = false;
                }
                return;
            }

            if (state.deltaOnly)
            {
                float newScroll = this->contentOffset - stateDelta;
                this->startScrolling(false, newScroll);
                return;
            }

            static float startOffset;
            if (state.state == GestureState::START)
            {
                if (Application::getInputType() != InputType::TOUCH)
                    Application::giveFocus(this);

                startOffset           = this->contentOffset;
                this->isRubberBanding = false;
            }

            float rawScroll  = startOffset - (statePos - stateStart);
            float contentLen = this->getContentLength();
            float endLimit   = contentLen - this->getScrollingAreaLength();

            float newScroll = rawScroll;
            if (endLimit > 0.0f)
            {
                if (rawScroll < 0.0f)
                {
                    newScroll             = rawScroll * 0.35f;
                    this->isRubberBanding = true;
                }
                else if (rawScroll > endLimit)
                {
                    newScroll             = endLimit + (rawScroll - endLimit) * 0.35f;
                    this->isRubberBanding = true;
                }
                else
                {
                    newScroll             = rawScroll;
                    this->isRubberBanding = false;
                }
            }
            else
            {
                newScroll = rawScroll;
            }

            if (state.state != GestureState::END)
            {
                this->startScrolling(false, newScroll);
            }
            else
            {
                float currentOffset = static_cast<float>(this->contentOffset);
                float clampMax      = std::max(0.0f, endLimit);
                bool outOfBounds    = (currentOffset < 0.0f) || (endLimit > 0.0f && currentOffset > endLimit);

                if (outOfBounds)
                {
                    float snapTarget = std::max(0.0f, std::min(currentOffset, clampMax));
                    this->contentOffset.setEndCallback([](bool) {});
                    this->contentOffset.stop();
                    this->contentOffset.reset();
                    this->isRubberBanding = true;
                    this->contentOffset.addStep(snapTarget, 300, EasingFunction::quadraticOut);
                    this->contentOffset.setTickCallback([this] { this->scrollAnimationTick(); });
                    this->contentOffset.setEndCallback(
                        [this](bool)
                        {
                            this->isRubberBanding = false;
                            this->invalidate();
                        }
                    );
                    this->contentOffset.start();
                    this->invalidate();
                    return;
                }

                this->isRubberBanding = false;

                float time   = accelTime * 1000.0f;
                float newPos = currentOffset + accelDist;
                newScroll    = newPos;

                if (newScroll == currentOffset || time < 50.0f)
                    return;

                newScroll = std::max(0.0f, std::min(newScroll, clampMax));
                this->animateScrolling(newScroll, time);
            }
        },
        panAxis
    ));

    // Stop scrolling on tap
    addGestureRecognizer(new TapGestureRecognizer(
        [this](brls::TapGestureStatus status, Sound*)
        {
            if (status.state == GestureState::UNSURE)
            {
                this->contentOffset.setEndCallback([](bool) {});
                this->contentOffset.stop();
            }
        }
    ));

    inputTypeSubscription = Application::getGlobalInputTypeChangeEvent()->subscribe(
        [this](InputType type)
        {
            if (!this->focused && !this->childFocused)
                return;

            if (this->behavior == ScrollingBehavior::NATURAL && type == InputType::GAMEPAD)
            {
                Application::giveFocus(this->getDefaultFocus());
                this->naturalScrollingCanScroll = false;
            }
        }
    );

    setHideHighlightBackground(true);
    setHideHighlightBorder(true);
}

void BaseScrollingFrame::setupScrollingIndicator()
{
    Theme theme        = Application::getTheme();
    scrollingIndicator = new Rectangle(theme["brls/scrolling_frame/indicator"]);

    if (scrollAxis == ScrollingAxis::VERTICAL)
        scrollingIndicator->setSize(Size(SCROLLING_INDICATOR_SIZE, 0));
    else
        scrollingIndicator->setSize(Size(0, SCROLLING_INDICATOR_SIZE));

    scrollingIndicator->setCornerRadius(SCROLLING_INDICATOR_SIZE / 2);
    scrollingIndicator->detach();
    Box::addView(scrollingIndicator);
}

void BaseScrollingFrame::updateScrollingIndicator()
{
    float contentLen = getContentLength();
    float viewLen    = (scrollAxis == ScrollingAxis::VERTICAL) ? getHeight() : getWidth();

    if (contentLen <= viewLen || !showScrollingIndicator)
    {
        scrollingIndicator->setAlpha(0);
        return;
    }

    scrollingIndicator->setAlpha(0.3f);
    float indicatorLen = viewLen / contentLen * viewLen;

    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        scrollingIndicator->setHeight(indicatorLen);
        float scrollViewOffset = getContentOffset() / contentLen * getHeight();
        scrollingIndicator->setDetachedPosition(getWidth() - 14 - SCROLLING_INDICATOR_SIZE, scrollViewOffset);
    }
    else
    {
        scrollingIndicator->setWidth(indicatorLen);
        float scrollViewOffset = getContentOffset() / contentLen * getWidth();
        scrollingIndicator->setDetachedPosition(scrollViewOffset, getHeight() - 14 - SCROLLING_INDICATOR_SIZE);
    }
}

void BaseScrollingFrame::rightStickScrolling()
{
    if (!contentView || !(focused || childFocused))
        return;

    ControllerState state {};
    input->updateUnifiedControllerState(&state);

    float stickVal = (scrollAxis == ScrollingAxis::VERTICAL) ? state.axes[RIGHT_Y] : state.axes[RIGHT_X];

    constexpr float deadzone = 0.15f;
    if (std::abs(stickVal) < deadzone)
        return;

    float normalized = (std::abs(stickVal) - deadzone) / (1.0f - deadzone);
    float speed      = normalized * normalized * 1200.0f / Application::getFPS();
    float newScroll  = getContentOffset() + (stickVal > 0 ? speed : -speed);

    float endLimit = getContentLength() - getScrollingAreaLength();
    newScroll      = std::max(0.0f, std::min(newScroll, std::max(0.0f, endLimit)));

    startScrolling(false, newScroll);
}

void BaseScrollingFrame::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    updateScrollingIndicator();
    naturalScrollingBehaviour();
    rightStickScrolling();

    if (updateScrollingOnNextFrame && updateScrolling(false))
        updateScrollingOnNextFrame = false;

    // Auto-correct: if content is out of bounds, not animating, and not rubber-banding, snap back instantly.
    if (contentView && !isRubberBanding && !contentOffset.isRunning())
    {
        float contentLen = getContentLength();
        float areaLen    = getScrollingAreaLength();
        float endLimit   = contentLen - areaLen;
        float offset     = static_cast<float>(contentOffset);
        float corrected  = contentLen <= areaLen ? 0.0f : std::max(0.0f, std::min(offset, std::max(0.0f, endLimit)));
        if (offset != corrected)
        {
            contentOffset = corrected;
            if (scrollAxis == ScrollingAxis::VERTICAL)
                contentView->setTranslationY(-corrected);
            else
                contentView->setTranslationX(-corrected);
        }
    }

    // Enable scissoring
    nvgSave(vg);
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        float scrollingTop    = getScrollingAreaStart();
        float scrollingHeight = getScrollingAreaLength();
        nvgIntersectScissor(vg, x, scrollingTop, getWidth(), scrollingHeight);
    }
    else
    {
        float scrollingLeft  = getScrollingAreaStart();
        float scrollingWidth = getScrollingAreaLength();
        nvgIntersectScissor(vg, scrollingLeft, y, scrollingWidth, getHeight());
    }

    Box::draw(vg, x, y, width, height, style, ctx);

    nvgRestore(vg);
}

void BaseScrollingFrame::naturalScrollingBehaviour()
{
    if (behavior != ScrollingBehavior::NATURAL || Application::getInputType() == InputType::TOUCH)
        return;

    if (focused || childFocused)
    {
        View* currentFocus = Application::getCurrentFocus();
        if (!currentFocus->getFrame().inscribed(getFrame()))
        {
            Application::giveFocus(this);
        }

        if (Application::getCurrentFocus() == this && Application::getInputType() == InputType::GAMEPAD)
        {
            View* edgeView = findEdgeFocusableView();

            if (edgeView && edgeView != currentFocus)
            {
                Application::giveFocus(edgeView);
                Application::getAudioPlayer()->play(Sound::SOUND_FOCUS_CHANGE);
            }
        }
    }

    if (!naturalScrollingCanScroll)
        return;

    if (focused || childFocused)
    {
        ControllerState state {};
        input->updateUnifiedControllerState(&state);
        float endLimit = getContentLength() - getScrollingAreaLength();

        static bool repeat = false;

        ControllerButton btnPositive, btnNegative;
        if (scrollAxis == ScrollingAxis::VERTICAL)
        {
            btnPositive = BUTTON_NAV_DOWN;
            btnNegative = BUTTON_NAV_UP;
        }
        else
        {
            btnPositive = BUTTON_NAV_RIGHT;
            btnNegative = BUTTON_NAV_LEFT;
        }

        if (state.buttons[btnPositive] && state.buttons[btnNegative])
            return;

        FocusDirection dirPositive = (scrollAxis == ScrollingAxis::VERTICAL) ? FocusDirection::DOWN : FocusDirection::RIGHT;
        FocusDirection dirNegative = (scrollAxis == ScrollingAxis::VERTICAL) ? FocusDirection::UP : FocusDirection::LEFT;

        if (state.buttons[btnPositive])
        {
            naturalScrollingButtonProcessing(dirPositive, &repeat);
        }

        if (state.buttons[btnNegative])
        {
            naturalScrollingButtonProcessing(dirNegative, &repeat);
        }

        View* currentFocus = Application::getCurrentFocus();
        if (!state.buttons[btnPositive] && !state.buttons[btnNegative] && (currentFocus != this))
        {
            naturalScrollingCanScroll = false;
        }

        if ((!state.buttons[btnPositive] && !state.buttons[btnNegative]) || (getContentOffset() > 0.01f && getContentOffset() < endLimit))
        {
            repeat = false;
        }
    }
}

View* BaseScrollingFrame::findEdgeFocusableView()
{
    Rect frame = getFrame();
    Point check;
    FocusDirection searchDir;

    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        check     = Point(frame.getMidX(), frame.getMinY());
        searchDir = FocusDirection::DOWN;
    }
    else
    {
        check     = Point(frame.getMinX(), frame.getMidY());
        searchDir = FocusDirection::RIGHT;
    }

    View* focusCheck = contentView->hitTest(check);
    if (focusCheck)
    {
        View* focusCheckDefaultFocus = focusCheck->getDefaultFocus();
        if (focusCheckDefaultFocus)
            focusCheck = focusCheckDefaultFocus;

        while (focusCheck && !focusCheck->getFrame().inscribed(frame))
        {
            focusCheck = focusCheck->getParent()->getNextFocus(searchDir, focusCheck);
        }

        return focusCheck;
    }

    return nullptr;
}

void BaseScrollingFrame::naturalScrollingButtonProcessing(FocusDirection focusDirection, bool* repeat)
{
    float endLimit  = getContentLength() - getScrollingAreaLength();
    float newOffset = getContentOffset();
    bool isBorder   = false;

    bool isNegative = (focusDirection == FocusDirection::UP || focusDirection == FocusDirection::LEFT);
    if (isNegative)
    {
        isBorder = getContentOffset() <= 0;
        newOffset -= (1000.0f / Application::getFPS());
    }
    else
    {
        isBorder = getContentOffset() >= endLimit;
        newOffset += (1000.0f / Application::getFPS());
    }

    setContentOffset(newOffset, false);
    View* current = Application::getCurrentFocus();
    View* next    = current->getParent()->getNextFocus(focusDirection, current);
    if (next)
    {
        if (current != next->getDefaultFocus())
        {
            Application::giveFocus(next);
            if (next != this)
                Application::getAudioPlayer()->play(SOUND_FOCUS_CHANGE);
        }
    }
    else if (!current->getFrame().inscribed(getFrame()))
    {
        Application::giveFocus(this);
    }

    if (isBorder && !*repeat)
    {
        *repeat = true;
        Application::getCurrentFocus()->shakeHighlight(focusDirection);
        Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
    }
}

void BaseScrollingFrame::addView(View* view) { setContentView(view); }

void BaseScrollingFrame::removeView(View* view, bool free) { setContentView(nullptr); }

void BaseScrollingFrame::setContentView(View* view)
{
    if (contentView)
    {
        Box::removeView(contentView);
        contentView = nullptr;
    }

    if (!view)
        return;

    contentView = view;

    view->detach();
    view->setCulled(false);

    if (scrollAxis == ScrollingAxis::VERTICAL)
        view->setWidth(getWidth());
    else
        view->setHeight(getHeight());

    Box::addView(view);
}

void BaseScrollingFrame::onLayout()
{
    if (contentView)
    {
        if (scrollAxis == ScrollingAxis::VERTICAL)
            contentView->setWidth(getWidth());
        else
            contentView->setHeight(getHeight());
        contentView->invalidate();
    }
}

float BaseScrollingFrame::getScrollingAreaStart() { return (scrollAxis == ScrollingAxis::VERTICAL) ? getY() : getX(); }

float BaseScrollingFrame::getScrollingAreaLength() { return (scrollAxis == ScrollingAxis::VERTICAL) ? getHeight() : getWidth(); }

void BaseScrollingFrame::willAppear(bool resetState)
{
    prebakeScrolling();

    if (resetState && behavior == ScrollingBehavior::CENTERED)
    {
        updateScrollingOnNextFrame = true;
    }

    Box::willAppear(resetState);
}

void BaseScrollingFrame::prebakeScrolling()
{
    float start  = getScrollingAreaStart();
    float length = getScrollingAreaLength();

    middlePos = start + length / 2;
    endPos    = start + length;
}

void BaseScrollingFrame::startScrolling(bool animated, float newScroll)
{
    if (newScroll == contentOffset)
        return;

    if (animated)
    {
        Style style = Application::getStyle();
        animateScrolling(newScroll, style["brls/animations/highlight"]);
    }
    else
    {
        contentOffset.setEndCallback([](bool) {});
        contentOffset.stop();
        contentOffset = newScroll;
        scrollAnimationTick();
        invalidate();
    }

    contentOffsetChanged.fire(newScroll);
}

void BaseScrollingFrame::animateScrolling(float newScroll, float time)
{
    contentOffset.setEndCallback([](bool) {});
    contentOffset.stop();
    contentOffset.reset();
    contentOffset.addStep(newScroll, time, EasingFunction::quadraticOut);
    contentOffset.setTickCallback([this] { this->scrollAnimationTick(); });
    contentOffset.start();

    invalidate();
}

void BaseScrollingFrame::setScrollingBehavior(ScrollingBehavior newBehavior) { behavior = newBehavior; }

float BaseScrollingFrame::getContentLength()
{
    if (!contentView)
        return 0;

    return (scrollAxis == ScrollingAxis::VERTICAL) ? contentView->getHeight() : contentView->getWidth();
}

void BaseScrollingFrame::setContentOffset(float value, bool animated) { startScrolling(animated, value); }

void BaseScrollingFrame::scrollAnimationTick()
{
    if (contentView)
    {
        float contentLen    = getContentLength();
        float endLimit      = contentLen - getScrollingAreaLength();
        float offset        = static_cast<float>(contentOffset);
        float displayOffset = offset;

        if (contentLen <= getScrollingAreaLength())
        {
            displayOffset = 0.0f;
        }
        else if (!isRubberBanding)
        {
            displayOffset = std::max(0.0f, std::min(offset, std::max(0.0f, endLimit)));
        }

        if (scrollAxis == ScrollingAxis::VERTICAL)
            contentView->setTranslationY(-displayOffset);
        else
            contentView->setTranslationX(-displayOffset);
    }
}

View* BaseScrollingFrame::getNextFocus(FocusDirection direction, View* currentView)
{
    float endLimit      = getContentLength() - getScrollingAreaLength();
    float currentOffset = getContentOffset();

    FocusDirection dirPositive = (scrollAxis == ScrollingAxis::VERTICAL) ? FocusDirection::DOWN : FocusDirection::RIGHT;
    FocusDirection dirNegative = (scrollAxis == ScrollingAxis::VERTICAL) ? FocusDirection::UP : FocusDirection::LEFT;

    if (direction == dirPositive && currentOffset < (endLimit - 0.01f))
        return this;

    if (direction == dirNegative && currentOffset > 0.01f)
        return this;

    return Box::getNextFocus(direction, currentView);
}

View* BaseScrollingFrame::getDefaultFocus()
{
    if (behavior == ScrollingBehavior::CENTERED)
    {
        View* focus = contentView->getDefaultFocus();
        if (focus)
            return focus;
        else
            return Box::getDefaultFocus();
    }

    View* focus = contentView->getDefaultFocus();
    if (focus && focus->getFrame().inscribed(getFrame()))
        return focus;

    if (focus = findEdgeFocusableView(); focus && focus != this)
        return focus;

    return Box::getDefaultFocus();
}

void BaseScrollingFrame::onFocusGained()
{
    Box::onFocusGained();
    naturalScrollingCanScroll = true;
}

void BaseScrollingFrame::onChildFocusGained(View* directChild, View* focusedView)
{
    Box::onChildFocusGained(directChild, focusedView);

    childFocused = true;

    if (Application::getInputType() == InputType::GAMEPAD && behavior == ScrollingBehavior::CENTERED)
        updateScrolling(true);
}

void BaseScrollingFrame::onChildFocusLost(View* directChild, View* focusedView) { childFocused = false; }

View* BaseScrollingFrame::getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction)
{
    if (behavior == ScrollingBehavior::CENTERED)
        return Box::getParentNavigationDecision(from, newFocus, direction);

    View* currentFocus = Application::getCurrentFocus();

    // Directions orthogonal to the scroll axis pass through
    FocusDirection orthDir1, orthDir2;
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        orthDir1 = FocusDirection::LEFT;
        orthDir2 = FocusDirection::RIGHT;
    }
    else
    {
        orthDir1 = FocusDirection::UP;
        orthDir2 = FocusDirection::DOWN;
    }

    if (!newFocus)
    {
        if (direction == orthDir1 || direction == orthDir2)
            return nullptr;

        if (from == contentView)
        {
            naturalScrollingCanScroll = true;
            if (currentFocus->getFrame().inscribed(getFrame()))
                return currentFocus;

            return this;
        }
        return nullptr;
    }
    else
    {
        if (newFocus->getFrame().inscribed(getFrame()))
            return newFocus;
        else
            naturalScrollingCanScroll = true;
    }

    if (currentFocus->getFrame().inscribed(getFrame()))
        return currentFocus;

    return this;
}

bool BaseScrollingFrame::updateScrolling(bool animated)
{
    if (!contentView)
        return false;

    View* focusedView = getDefaultFocus();

    float localPos;
    float itemLen;
    View* parent;

    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        localPos = focusedView ? focusedView->getLocalY() : 0.0f;
        itemLen  = focusedView ? focusedView->getHeight() : 0.0f;
    }
    else
    {
        localPos = focusedView ? focusedView->getLocalX() : 0.0f;
        itemLen  = focusedView ? focusedView->getWidth() : 0.0f;
    }

    parent = focusedView ? focusedView->getParent() : nullptr;

    while (parent && dynamic_cast<BaseScrollingFrame*>(parent->getParent()) == nullptr)
    {
        if (scrollAxis == ScrollingAxis::VERTICAL)
            localPos += parent->getLocalY();
        else
            localPos += parent->getLocalX();
        parent = parent->getParent();
    }

    float areaLen   = getScrollingAreaLength();
    float newScroll = (localPos + itemLen / 2) - areaLen / 2;

    float contentLen = getContentLength();
    float endLimit   = contentLen - areaLen;

    if (newScroll < 0)
        newScroll = 0;

    if (newScroll > endLimit)
        newScroll = endLimit;

    if (contentLen <= areaLen)
        newScroll = 0;

    startScrolling(animated, newScroll);

    return true;
}

Rect BaseScrollingFrame::getVisibleFrame()
{
    Rect frame = getLocalFrame();
    if (scrollAxis == ScrollingAxis::VERTICAL)
        frame.origin.y += contentOffset;
    else
        frame.origin.x += contentOffset;
    return frame;
}

enum Sound BaseScrollingFrame::getFocusSound()
{
    if (!contentView->getDefaultFocus())
    {
        return Box::getFocusSound();
    }
    return Sound::SOUND_NONE;
}

#define NO_PADDING(axis) fatal("Padding is not supported by brls:" axis "ScrollingFrame, please set padding on the content view instead");

void BaseScrollingFrame::setPadding(float top, float right, float bottom, float left)
{
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        NO_PADDING("")
    }
    else
    {
        NO_PADDING("H")
    }
}

void BaseScrollingFrame::setPaddingTop(float top)
{
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        NO_PADDING("")
    }
    else
    {
        NO_PADDING("H")
    }
}

void BaseScrollingFrame::setPaddingRight(float right)
{
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        NO_PADDING("")
    }
    else
    {
        NO_PADDING("H")
    }
}

void BaseScrollingFrame::setPaddingBottom(float bottom)
{
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        NO_PADDING("")
    }
    else
    {
        NO_PADDING("H")
    }
}

void BaseScrollingFrame::setPaddingLeft(float left)
{
    if (scrollAxis == ScrollingAxis::VERTICAL)
    {
        NO_PADDING("")
    }
    else
    {
        NO_PADDING("H")
    }
}

BaseScrollingFrame::~BaseScrollingFrame() { Application::getGlobalInputTypeChangeEvent()->unsubscribe(inputTypeSubscription); }

// ScrollingFrame (vertical)

ScrollingFrame::ScrollingFrame() : BaseScrollingFrame(ScrollingAxis::VERTICAL) {}

View* ScrollingFrame::create() { return new ScrollingFrame(); }

// HScrollingFrame (horizontal)

HScrollingFrame::HScrollingFrame() : BaseScrollingFrame(ScrollingAxis::HORIZONTAL) {}

View* HScrollingFrame::create() { return new HScrollingFrame(); }

} // namespace brls
