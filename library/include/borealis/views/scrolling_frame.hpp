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

#pragma once

#include <borealis/core/animation.hpp>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/rectangle.hpp>

namespace brls
{

enum class ScrollingBehavior
{
    // Inputs scroll the view like the scroll wheel on a web page, focus changes only when the next view to focus is fully on screen
    // To work properly, there must be at least one focusable view in the "top" area of the frame (there should not be the need to scroll to
    // see it)
    NATURAL,

    // The focused view is always in the center, inputs always change focus and scroll immediately
    CENTERED,

    // The top-most focusable ancestor of the focused view is aligned to the start (top, or left for horizontal
    // frames) of the scrolling area. Inputs always change focus and scroll immediately. Useful for sectioned
    // layouts where focusing any item within a section brings the entire section into view from its start.
    START,

    // The top-most focusable ancestor of the focused view is aligned to the end (bottom, or right for horizontal
    // frames) of the scrolling area. Inputs always change focus and scroll immediately. Useful for sectioned
    // layouts where focusing any item within a section brings the entire section into view from its end.
    END,
};

enum class ScrollingAxis
{
    VERTICAL,
    HORIZONTAL,
};

/**
 * Base class for scrolling frames. Implements all scrolling logic generically
 * over an axis (vertical or horizontal). Use ScrollingFrame or HScrollingFrame
 * for concrete vertical/horizontal instances.
 */
class BaseScrollingFrame : public Box
{
  public:
    /**
     * Creates a scrolling frame with the given axis direction.
     */
    explicit BaseScrollingFrame(ScrollingAxis axis);
    ~BaseScrollingFrame();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onFocusGained() override;
    void onChildFocusGained(View* directChild, View* focusedView) override;
    void onChildFocusLost(View* directChild, View* focusedView) override;
    void willAppear(bool resetState) override;
    void addView(View* view) override;
    void removeView(View* view, bool free = true) override;
    void onLayout() override;
    void setPadding(float top, float right, float bottom, float left) override;
    void setPaddingTop(float top) override;
    void setPaddingRight(float right) override;
    void setPaddingBottom(float bottom) override;
    void setPaddingLeft(float left) override;
    View* getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction) override;
    View* getNextFocus(FocusDirection direction, View* currentView) override;
    View* getDefaultFocus() override;
    enum Sound getFocusSound() override;

    /**
     * Returns the rectangle representing the currently visible portion of the content.
     */
    Rect getVisibleFrame();

    /**
     * Sets the content view of this scrolling box. There can only be one
     * content view per scrolling box at a time.
     */
    void setContentView(View* view);

    /**
     * Sets the scrolling behavior of this scrolling frame.
     * Default is NATURAL.
     */
    void setScrollingBehavior(ScrollingBehavior newBehavior);

    /**
     * The point at which the origin of the content view is offset from the origin of the scroll view.
     */
    float getContentOffset() const { return contentOffset; }

    /**
     * Sets the offset from the content view's origin that corresponds to the receiver's origin.
     */
    void setContentOffset(float value, bool animated);

    /**
     * Sets whether the scrolling indicator bar is visible.
     */
    void setScrollingIndicatorVisible(bool visible) { showScrollingIndicator = visible; }

    /**
     * Returns the event that is triggered when the content offset changes.
     */
    Event<float>* getContentOffsetChanged() { return &contentOffsetChanged; }

  protected:
    /** The axis along which this frame scrolls. */
    ScrollingAxis scrollAxis;

    /** The single child view whose content is scrolled. */
    View* contentView = nullptr;

    /** The visual scrollbar indicator rectangle. */
    Rectangle* scrollingIndicator = nullptr;

    /** When true, scrolling position will be recalculated on the next frame. */
    bool updateScrollingOnNextFrame = false;

    /** Whether a child of this frame currently holds focus. */
    bool childFocused = false;

    /** Whether the scrolling indicator bar is shown. */
    bool showScrollingIndicator = true;

    /** Scroll position at which the content is centered in the frame. */
    float middlePos = 0;

    /** Maximum scroll position (content fully scrolled to the end). */
    float endPos = 0;

    /** Current animated scroll offset value. */
    Animatable contentOffset = 0.0f;

    /** Pre-computes middlePos and endPos based on current layout. */
    void prebakeScrolling();

    /** Updates the scroll position to follow focus. Returns true if scrolling changed. */
    bool updateScrolling(bool animated);

    /** Initiates a scroll to the given offset, optionally animated. */
    void startScrolling(bool animated, float newScroll);

    /** Animates the scroll offset to a new value over the given time. */
    void animateScrolling(float newScroll, float time);

    /** Per-frame tick callback that drives the scroll animation. */
    void scrollAnimationTick();

    /** Returns the start position of the scrollable area (accounting for padding). */
    float getScrollingAreaStart();

    /** Returns the length of the scrollable area (accounting for padding). */
    float getScrollingAreaLength();

    /** Returns the total length of the content view along the scroll axis. */
    float getContentLength();

    /** The current scrolling behavior (NATURAL or CENTERED). */
    ScrollingBehavior behavior = ScrollingBehavior::NATURAL;

    /** Cached pointer to the application's input manager. */
    InputManager* input;

    /** Whether natural scrolling mode currently allows scroll input. */
    bool naturalScrollingCanScroll = false;

    /** Whether the view is currently in a rubber-banding overscroll state. */
    bool isRubberBanding = false;

    /** Handles directional button scrolling in NATURAL behavior mode. */
    void naturalScrollingBehaviour();

    /** Processes a single directional button press for natural scrolling. */
    void naturalScrollingButtonProcessing(FocusDirection focusDirection, bool* repeat);

    /** Handles right analog stick continuous scrolling. */
    void rightStickScrolling();

    /** Finds the focusable view closest to the edge in the scroll direction. */
    View* findEdgeFocusableView();

    /** Creates and adds the scrolling indicator rectangle to the view hierarchy. */
    void setupScrollingIndicator();

    /** Updates the position and size of the scrolling indicator to reflect current scroll state. */
    void updateScrollingIndicator();

    /** Subscription handle for input type change events. */
    Event<InputType>::Subscription inputTypeSubscription;

    /** Event fired whenever the content offset changes. */
    Event<float> contentOffsetChanged;
};

/**
 * A vertical-only frame that can scroll if its content overflows.
 */
class ScrollingFrame : public BaseScrollingFrame
{
  public:
    ScrollingFrame();

    /** Returns the current vertical content offset. */
    float getContentOffsetY() const { return getContentOffset(); }

    /** Sets the vertical content offset, optionally with animation. */
    void setContentOffsetY(float value, bool animated) { setContentOffset(value, animated); }

    /** XML element creator for registration with the view system. */
    static View* create();
};

/**
 * A horizontal-only frame that can scroll if its content overflows.
 */
class HScrollingFrame : public BaseScrollingFrame
{
  public:
    HScrollingFrame();

    /** Returns the current horizontal content offset. */
    float getContentOffsetX() const { return getContentOffset(); }

    /** Sets the horizontal content offset, optionally with animation. */
    void setContentOffsetX(float value, bool animated) { setContentOffset(value, animated); }

    /** XML element creator for registration with the view system. */
    static View* create();
};

} // namespace brls
