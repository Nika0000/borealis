#pragma once

#include <borealis/core/event.hpp>
#include <borealis/core/gesture.hpp>

namespace brls
{

struct HoverGestureConfig
{
    bool highlightOnHover = true;
    CursorType cursor     = CursorType::CURSOR_POINTER;

    HoverGestureConfig() = default;
    HoverGestureConfig(bool highlightOnHover, CursorType cursor)
    {
        this->highlightOnHover = highlightOnHover;
        this->cursor           = cursor;
    }
};

struct HoverGestureStatus
{
    GestureState state; // Gesture state
    Point position; // Current cursor position
    Point delta; // Difference between current and previous positions
};

typedef Event<HoverGestureStatus> HoverGestureEvent;

// Mouse hover recognizer
// START: first frame when the cursor is over the view without any buttons pressed
// STAY: subsequent frames while the cursor remains over the view (even if idle)
// END: when the cursor leaves the view (synthetic signal)
class HoverGestureRecognizer : public GestureRecognizer
{
  public:
    HoverGestureRecognizer(View* view, HoverGestureConfig config = HoverGestureConfig());
    HoverGestureRecognizer(HoverGestureEvent::Callback respond);
    ~HoverGestureRecognizer();

    GestureState recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay) override;

    // Get current status of recognizer
    HoverGestureStatus getCurrentStatus() const;

    // Get hover gesture event
    HoverGestureEvent getHoverGestureEvent() const { return hoverEvent; }

  private:
    HoverGestureEvent hoverEvent;
    Point position;
    Point lastPosition;
    Point delta;
    GestureState lastState = GestureState::FAILED;
};

} // namespace brls