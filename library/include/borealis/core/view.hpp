/*
    Copyright 2019-2021 natinusala
    Copyright 2019 p-sam

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

#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <nanovg.h>
#include <stdio.h>
#include <tinyxml2.h>
#include <yoga/Yoga.h>

#include <borealis/core/actions.hpp>
#include <borealis/core/animation.hpp>
#include <borealis/core/event.hpp>
#include <borealis/core/frame_context.hpp>
#include <borealis/core/geometry.hpp>
#include <borealis/core/gesture.hpp>
#include <borealis/core/util.hpp>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#endif

// Registers an "enum" XML attribute, which is just a string attribute with a
// map string -> enum inside When using this macro please use the same (wonky)
// formatting as what you see in Box.cpp or View.cpp otherwise clang-format will
// screw it up
#define BRLS_REGISTER_ENUM_XML_ATTRIBUTE(name, enumType, method, ...)                                                                      \
    this->registerStringXMLAttribute(                                                                                                      \
        name,                                                                                                                              \
        [this](const std::string& value)                                                                                                   \
        {                                                                                                                                  \
            std::unordered_map<std::string, enumType> enumMap = __VA_ARGS__;                                                               \
            if (enumMap.count(value) > 0)                                                                                                  \
                method(enumMap[value]);                                                                                                    \
            else                                                                                                                           \
                brls::fatal("Illegal value \"" + value + "\" for XML attribute \"" + name + "\"");                                         \
        }                                                                                                                                  \
    )

// Shortcut to register an A key action (generic click) on a view given its id,
// that calls any function or method To be used in activities or derivates of
// Box (internally uses the getView() method) The function or method must return
// a boolean (true if the action was consumed, false otherwise) and take a
// single brls::View* parameter (useful to know what was clicked if you use the
// same listener for multiple views)
#define BRLS_REGISTER_CLICK_BY_ID(id, method) this->getView(id)->registerClickAction([this](brls::View* view) { return method(view); })

#define ASYNC_RETAIN                                                                                                                       \
    if (!deletionToken && !deletionTokenCounter)                                                                                           \
    {                                                                                                                                      \
        deletionToken        = new bool(false);                                                                                            \
        deletionTokenCounter = new int(0);                                                                                                 \
    }                                                                                                                                      \
    (*deletionTokenCounter)++;                                                                                                             \
    bool* token       = deletionToken;                                                                                                     \
    int* tokenCounter = deletionTokenCounter;

#define ASYNC_RELEASE                                                                                                                      \
    bool release = *token;                                                                                                                 \
    int counter  = *tokenCounter;                                                                                                          \
    if (counter > 0)                                                                                                                       \
    {                                                                                                                                      \
        (*tokenCounter)--;                                                                                                                 \
        if (*tokenCounter == 0)                                                                                                            \
        {                                                                                                                                  \
            delete token;                                                                                                                  \
            delete tokenCounter;                                                                                                           \
            if (!release)                                                                                                                  \
            {                                                                                                                              \
                deletionToken        = nullptr;                                                                                            \
                deletionTokenCounter = nullptr;                                                                                            \
            }                                                                                                                              \
        }                                                                                                                                  \
    }                                                                                                                                      \
    if (release)                                                                                                                           \
        return;

#define ASYNC_TOKEN this, token, tokenCounter

// Avoid conflicts with macro definitions in windows.h
#undef RGB
#undef TRANSPARENT
#undef RELATIVE
#undef ABSOLUTE

namespace brls
{

inline const NVGcolor TRANSPARENT = nvgRGBA(0, 0, 0, 0);

// Focus direction when navigating
enum class FocusDirection
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// View background
enum class ViewBackground
{
    NONE,
    SIDEBAR,
    BACKDROP,
    SHAPE_COLOR,
    VERTICAL_LINEAR,
    HORIZONTAL_LINEAR,
};

enum class AlignSelf
{
    AUTO,
    FLEX_START,
    CENTER,
    FLEX_END,
    STRETCH,
    BASELINE,
    SPACE_BETWEEN,
    SPACE_AROUND,
};

// View visibility
enum class Visibility
{
    VISIBLE,   // the view is visible
    INVISIBLE, // the view is invisible but still takes some space
    GONE,      // the view is invisible and doesn't take any space
};

// Position attribute behavior
enum class PositionType
{
    RELATIVE, // position attributes offset the view from the base layout
    ABSOLUTE, // position attributes set the absolute coordinates of the view
};

// The animation to play when
// pushing / popping an activity or
// showing / hiding a view.
enum class TransitionAnimation
{
    FADE,        // the old activity fades away and the new one fades in
    SLIDE_LEFT,  // the old activity slides out to the left and the new one slides in from the right
    SLIDE_RIGHT, // inverted SLIDE_LEFT
    NONE,
    LINEAR,
};

// A View shape's shadow type
enum class ShadowType
{
    NONE,    // do not draw any shadow around the shape
    GENERIC, // generic all-purpose shadow
    CUSTOM,  // customized shadow (use the provided methods to tweak it)
};

struct AppletFrameItem
{
    std::string title;
    std::string iconPath;

    void setIconFromRes(const std::string& name)
    {
#ifdef USE_LIBROMFS
        iconPath = "@res/" + name;
#else
        iconPath = std::string(BRLS_RESOURCES) + name;
#endif
    }

    void setIconFromFile(std::string path) { iconPath = std::move(path); }

    void setHintView(View* hintView);
    View* getHintView() { return m_hintView; }

    ~AppletFrameItem();

  private:
    View* m_hintView = nullptr;
};

extern const NVGcolor transparent;

class View;
class Box;
class AppletFrame;
class Activity;

typedef Event<View*> GenericEvent;
typedef Event<> VoidEvent;

typedef std::function<void(void)> AutoAttributeHandler;
typedef std::function<void(int)> IntAttributeHandler;
typedef std::function<void(float)> FloatAttributeHandler;
typedef std::function<void(std::string)> StringAttributeHandler;
typedef std::function<void(NVGcolor)> ColorAttributeHandler;
typedef std::function<void(bool)> BoolAttributeHandler;
typedef std::function<void(std::string)> FilePathAttributeHandler;

/**
 * Some YG values are NAN if not set, wrecking our
 * calculations if we use them as they are
 */
float ntz(float value);

// Superclass for all the other views
// Lifecycle of a view is :
//   new -> [willAppear -> willDisappear] -> delete
//
// Users have do to the new, the rest of the lifecycle is taken
// care of by the library
//
// willAppear and willDisappear can be called zero or multiple times
// before deletion (in case of a TabLayout for instance)
class View
{
  private:
    ViewBackground m_background = ViewBackground::NONE;

    void drawBackground(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame);
    void drawShadow(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame);
    void drawBorder(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame);
    void drawHighlight(NVGcontext* vg, Theme theme, float alpha, Style style, bool background);
    void drawClickAnimation(NVGcontext* vg, FrameContext* ctx, Rect frame);
    void drawWireframe(FrameContext* ctx, Rect frame);
    void drawLine(FrameContext* ctx, Rect frame);

    Animatable m_highlightAlpha   = 0.0f;
    float m_highlightPadding      = 0.0f;
    float m_highlightCornerRadius = 0.0f;

    Animatable m_clickAlpha = 0.0f; // animated between 0 and 1

    bool m_highlightShaking = false;
    Time m_highlightShakeStart;
    FocusDirection m_highlightShakeDirection;
    float m_highlightShakeAmplitude;

    bool m_fadeIn          = false; // is the fade in animation running?
    bool m_inFadeAnimation = false; // is any fade animation running?

    Theme* m_themeOverride = nullptr;

    bool m_hidden    = false;
    bool m_focusable = false;

    enum Sound m_focusSound = SOUND_FOCUS_CHANGE;

    bool m_hideHighlightBackground = false;
    bool m_hideHighlightBorder     = false;
    bool m_hideHighlight           = false;
    bool m_hideClickAnimation      = false;

    bool m_detached = false;
    Point m_detachedOrigin;

    Point m_translation;
    Point m_scale;

    bool m_wireframeEnabled = false;
    bool m_clipsToBounds    = false;

    std::vector<Action> m_actions;
    std::vector<GestureRecognizer*> m_gestureRecognizers;

    /**
     * Parent user data, typically the index of the view
     * in the internal layout structure
     */
    void* m_parentUserdata = nullptr;

    bool m_culled = true; // will be culled by the parent Box, if any

    float m_aspectRatio = 0;

    std::unordered_map<std::string, AutoAttributeHandler> m_autoAttributes;
    std::unordered_map<std::string, FloatAttributeHandler> m_percentageAttributes;
    std::unordered_map<std::string, FloatAttributeHandler> m_floatAttributes;
    std::unordered_map<std::string, StringAttributeHandler> m_stringAttributes;
    std::unordered_map<std::string, ColorAttributeHandler> m_colorAttributes;
    std::unordered_map<std::string, BoolAttributeHandler> m_boolAttributes;
    std::unordered_map<std::string, FilePathAttributeHandler> m_filePathAttributes;

    std::set<std::string> m_knownAttributes;

    void registerCommonAttributes();
    void printXMLAttributeErrorMessage(tinyxml2::XMLElement* element, const std::string& name, const std::string& value);

    unsigned m_maximumAllowedXMLElements = UINT_MAX;

    NVGcolor m_lineColor = TRANSPARENT;
    float m_lineTop      = 0;
    float m_lineRight    = 0;
    float m_lineBottom   = 0;
    float m_lineLeft     = 0;

    Visibility m_visibility = Visibility::VISIBLE;

    NVGcolor m_backgroundColor = TRANSPARENT;

    // Background gradient colors for vertical linear style
    NVGcolor m_backgroundStartColor = TRANSPARENT;
    NVGcolor m_backgroundEndColor   = nvgRGBA(0, 0, 0, 200);

    // Background corner radii for vertical linear
    // style: top-left, top-right, bottom-right, bottom-left
    std::array<float, 4> m_backgroundRadius { 0.0f, 0.0f, 0.0f, 0.0f };

    NVGcolor m_borderColor             = TRANSPARENT;
    float m_borderThickness            = 0.0f;
    float m_cornerRadius               = 0.0f;
    std::array<float, 4> m_cornerRadii = { 0.0f, 0.0f, 0.0f, 0.0f }; // TL, TR, BR, BL
    ShadowType m_shadowType            = ShadowType::NONE;
    bool m_showShadow                  = true;

    std::unordered_map<FocusDirection, std::string> m_customFocusById;
    std::unordered_map<FocusDirection, View*> m_customFocusByPtr;

    AppletFrameItem m_appletFrameItem;

    int m_ptrLockCounter = 0;

  protected:
    Animatable collapseState = 1.0f;

    bool focused = false;

    Activity* parentActivity = nullptr;
    Box* parent              = nullptr;

    GenericEvent focusEvent;
    GenericEvent focusLostEvent;

    YGNode* ygNode;

    std::string id;

    // Helper functions to apply this view's alpha to a color
    NVGcolor a(NVGcolor color);
    NVGpaint a(NVGpaint paint);

    NVGcolor RGB(unsigned r, unsigned g, unsigned b) { return this->a(nvgRGB(r, g, b)); }

    NVGcolor RGBA(unsigned r, unsigned g, unsigned b, unsigned a) { return this->a(nvgRGBA(r, g, b, a)); }

    NVGcolor RGBf(float r, float g, float b) { return this->a(nvgRGBf(r, g, b)); }

    NVGcolor RGBAf(float r, float g, float b, float a) { return this->a(nvgRGBAf(r, g, b, a)); }

    /**
     * Should the hint alpha be animated when
     * pushing the view?
     */
    virtual bool animateHint() { return false; }

  public:
    /**
     * Special value used for width/height/margin/position to indicate
     * that the layout engine should determine the value automatically.
     */
    static constexpr float AUTO = NAN;

    /**
     * Custom resources path override. When set, XML and resource loading
     * will check this path before falling back to the default resources.
     */
    inline static std::string CUSTOM_RESOURCES_PATH;

    View();
    virtual ~View();

    /**
     * Sets the background type of the view.
     */
    void setBackground(ViewBackground background);

    /**
     * Sets a linear gradient background for the view.
     */
    void setBackgroundLinearGradient(const NVGcolor start, const NVGcolor end, ViewBackground direction = ViewBackground::VERTICAL_LINEAR);

    /**
     * Plays a shake animation on the highlight in the given direction.
     * Used to indicate that navigation in that direction is not possible.
     */
    void shakeHighlight(FocusDirection direction);

    /**
     * Returns the absolute frame (position and size) of the view.
     */
    Rect getFrame();

    /**
     * Returns the absolute X position of the view.
     */
    float getX();

    /**
     * Returns the absolute Y position of the view.
     */
    float getY();

    /**
     * Returns the local frame (position relative to parent and size) of the view.
     */
    Rect getLocalFrame();

    /**
     * Returns the X position of the view relative to its parent.
     */
    float getLocalX();

    /**
     * Returns the Y position of the view relative to its parent.
     */
    float getLocalY();

    /**
     * Returns the width of the view as computed by the layout engine.
     */
    float getWidth();

    /**
     * Returns the height of the view as computed by the layout engine.
     * @param includeCollapse if true, the height is scaled by the collapse state.
     */
    float getHeight(bool includeCollapse = true);

    /**
     * Triggers a layout of the whole view tree. Must be called
     * after a yoga node property is changed.
     *
     * Only methods that change yoga nodes properties should
     * call this method.
     */
    virtual void invalidate();

    /**
     * Called when a layout pass ends on that view.
     */
    virtual void onLayout() {};

    /**
     * Returns the view with the corresponding id in the view or its children,
     * or nullptr if it hasn't been found.
     *
     * Research is done recursively by traversing the tree starting from this
     * view. This view's parents are not traversed.
     */
    virtual View* getView(const std::string& id);

    // -----------------------------------------------------------
    // Flex layout properties
    // -----------------------------------------------------------

    /**
     * Sets the preferred width of the view. Use brls::View::AUTO
     * to have the layout automatically resize the view.
     *
     * If set to anything else than AUTO, the view is guaranteed
     * to never shrink below the given width.
     */
    void setWidth(float width);

    /**
     * Sets the preferred height of the view. Use brls::View::AUTO
     * to have the layout automatically resize the view.
     *
     * If set to anything else than AUTO, the view is guaranteed
     * to never shrink below the given height.
     */
    void setHeight(float height);

    /**
     * Sets the preferred width and height of the view. Use brls::View::AUTO
     * to have the layout automatically resize the view.
     *
     * If set to anything else than AUTO, the view is guaranteed
     * to never shrink below the given height.
     */
    void setSize(Size size);

    /**
     * Shortcut to setWidth + setHeight.
     *
     * Only does one layout pass instead of two when using the two methods
     * separately.
     */
    void setDimensions(float width, float height);

    /**
     * Sets the preferred width of the view in percentage of
     * the parent view width. Between 0.0f and 100.0f.
     */
    void setWidthPercentage(float percentage);

    /**
     * Sets the preferred height of the view in percentage of
     * the parent view height. Between 0.0f and 100.0f.
     */
    void setHeightPercentage(float percentage);

    /**
     * Sets the minimum width of the view, in pixels.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be less than the given min width.
     *
     * Use View::AUTO to disable the min width constraint.
     */
    void setMinWidth(float minWidth);

    /**
     * Sets the minimum height of the view, in pixels.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be less than the given max height.
     *
     * Use View::AUTO to disable the min height constraint.
     */
    void setMinHeight(float minHeight);

    /**
     * Sets the minimum width of the view, in parent width percentage.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be less than the given max width.
     *
     * Use View::AUTO to disable the min width constraint.
     */
    void setMinWidthPercentage(float percentage);

    /**
     * Sets the minimum height of the view, in parent height percentage.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be less than the given max height.
     *
     * Use View::AUTO to disable the min height constraint.
     */
    void setMinHeightPercentage(float percentage);

    /**
     * Sets the maximum width of the view, in pixels.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be larger than the given max width.
     *
     * Use View::AUTO to disable the max width constraint.
     */
    void setMaxWidth(float maxWidth);

    /**
     * Sets the maximum height of the view, in pixels.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be larger than the given max height.
     *
     * Use View::AUTO to disable the max height constraint.
     */
    void setMaxHeight(float maxHeight);

    /**
     * Sets the maximum width of the view, in parent width percentage.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be larger than the given max width.
     *
     * Use View::AUTO to disable the max width constraint.
     */
    void setMaxWidthPercentage(float percentage);

    /**
     * Sets the maximum height of the view, in parent height percentage.
     *
     * This constraint is stronger than the grow factor: the view
     * is guaranteed to never be larger than the given max height.
     *
     * Use View::AUTO to disable the max height constraint.
     */
    void setMaxHeightPercentage(float percentage);

    /**
     * Sets the grow factor of the view, aka the percentage
     * of remaining space to give this view, in the containing box axis.
     * Opposite of shrink.
     * Default is 0.0f;
     */
    void setGrow(float grow);

    /**
     * Sets the shrink factor of the view, aka the percentage of space
     * the view is allowed to shrink for if there is not enough space for everyone
     * in the contaning box axis. Opposite of grow.
     * Default is 1.0f;
     */
    void setShrink(float shrink);

    /**
     * Sets the margin of the view, aka the space that separates
     * this view and the surrounding ones in all 4 directions.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Only does one layout pass instead of four when using the four methods
     * separately.
     */
    void setMargins(float margin);

    /**
     * Sets the margin of the view, aka the space that separates
     * this view and the surrounding ones in all 4 directions.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Only does one layout pass instead of four when using the four methods
     * separately.
     */
    void setMargins(float top, float right, float bottom, float left);

    /**
     * Sets the top margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginTop(float top);

    /**
     * Sets the right margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginRight(float right);

    /**
     * Returns the right margin of the view.
     */
    float getMarginRight();

    /**
     * Returns the left margin of the view.
     */
    float getMarginLeft();

    /**
     * Sets the bottom margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginBottom(float bottom);

    /**
     * Sets the left margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginLeft(float left);

    /**
     * Sets the visibility of the view.
     */
    void setVisibility(Visibility visibility);

    /**
     * Gets the visibility of the view.
     */
    Visibility getVisibility();

    /**
     * Sets the top position of the view, in pixels.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     *
     * If relative, it will simply offset the view by the given amount.
     *
     * If absolute, it will behave like the "display: absolute;" CSS property
     * and move the view freely in its parent. Use 0 to snap to the parent top
     * edge. Absolute positioning ignores padding.
     *
     * Use View::AUTO to disable (not the same as 0).
     */
    void setPositionTop(float pos);

    /**
     * Sets the right position of the view, in pixels.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     *
     * If relative, it will simply offset the view by the given amount.
     *
     * If absolute, it will behave like the "display: absolute;" CSS property
     * and move the view freely in its parent. Use 0 to snap to the parent right
     * edge. Absolute positioning ignores padding.
     *
     * Use View::AUTO to disable (not the same as 0).
     */
    void setPositionRight(float pos);

    /**
     * Sets the bottom position of the view, in pixels.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     *
     * If relative, it will simply offset the view by the given amount.
     *
     * If absolute, it will behave like the "display: absolute;" CSS property
     * and move the view freely in its parent. Use 0 to snap to the parent bottom
     * edge. Absolute positioning ignores padding.
     *
     * Use View::AUTO to disable (not the same as 0).
     */
    void setPositionBottom(float pos);

    /**
     * Sets the left position of the view, in pixels.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     *
     * If relative, it will simply offset the view by the given amount.
     *
     * If absolute, it will behave like the "display: absolute;" CSS property
     * and move the view freely in its parent. Use 0 to snap to the parent left
     * edge. Absolute positioning ignores padding.
     *
     * Use View::AUTO to disable (not the same as 0).
     */
    void setPositionLeft(float pos);

    /**
     * Sets the top position of the view, in percents.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     */
    void setPositionTopPercentage(float percentage);

    /**
     * Sets the right position of the view, in percents.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     */
    void setPositionRightPercentage(float percentage);

    /**
     * Sets the bottom position of the view, in percentage.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     */
    void setPositionBottomPercentage(float percentage);

    /**
     * Sets the left position of the view, in percents.
     *
     * The behavior of this attribute changes depending on the
     * position type of the view.
     */
    void setPositionLeftPercentage(float percentage);

    /**
     * Sets the "position type" of the view, aka the behavior
     * of the 4 position attributes.
     *
     * Default is RELATIVE.
     */
    void setPositionType(PositionType type);

    /**
     * Sets the id of the view.
     */
    void setId(const std::string& id);

    /**
     * Overrides align items of the parent box.
     *
     * Default is AUTO.
     */
    void setAlignSelf(AlignSelf align);

    // -----------------------------------------------------------
    // Styling and view shape properties
    // -----------------------------------------------------------

    /**
     * Sets the line color for the view. To be used with setLineTop(),
     * setLineRight()...
     *
     * The "line" is separate from the shape "border".
     */
    inline void setLineColor(NVGcolor color) { this->m_lineColor = color; }

    /**
     * Sets the top line thickness. Use setLineColor()
     * to change the line color.
     *
     * The "line" is separate from the shape "border".
     */
    inline void setLineTop(float thickness) { this->m_lineTop = thickness; }

    /**
     * Sets the right line thickness. Use setLineColor()
     * to change the line color.
     *
     * The "line" is separate from the shape "border".
     */
    inline void setLineRight(float thickness) { this->m_lineRight = thickness; }

    /**
     * Sets the bottom line thickness. Use setLineColor()
     * to change the line color.
     *
     * The "line" is separate from the shape "border".
     */
    inline void setLineBottom(float thickness) { this->m_lineBottom = thickness; }

    /**
     * Sets the left line thickness. Use setLineColor()
     * to change the line color.
     *
     * The "line" is separate from the shape "border".
     */
    inline void setLineLeft(float thickness) { this->m_lineLeft = thickness; }

    /**
     * Sets the view shape background color.
     */
    inline void setBackgroundColor(NVGcolor color)
    {
        this->m_backgroundColor = color;
        this->setBackground(ViewBackground::SHAPE_COLOR);
    }

    /**
     * Sets the view shape border color.
     */
    inline void setBorderColor(NVGcolor color) { this->m_borderColor = color; }

    /**
     * Sets the view shape border thickness.
     */
    inline void setBorderThickness(float thickness) { this->m_borderThickness = thickness; }

    /**
     * Returns the view shape border thickness.
     */
    inline float getBorderThickness() const { return this->m_borderThickness; }

    /**
     * Sets the view shape corner radius (uniform).
     * 0 means no rounded corners.
     */
    inline void setCornerRadius(float radius)
    {
        this->m_cornerRadius   = radius;
        this->m_cornerRadii[0] = radius;
        this->m_cornerRadii[1] = radius;
        this->m_cornerRadii[2] = radius;
        this->m_cornerRadii[3] = radius;
    }

    /**
     * Sets the view shape corner radii individually.
     * Order: top-left, top-right, bottom-right, bottom-left.
     */
    inline void setCornerRadius(float topLeft, float topRight, float bottomRight, float bottomLeft)
    {
        this->m_cornerRadii[0] = topLeft;
        this->m_cornerRadii[1] = topRight;
        this->m_cornerRadii[2] = bottomRight;
        this->m_cornerRadii[3] = bottomLeft;
        this->m_cornerRadius   = (topLeft == topRight && topRight == bottomRight && bottomRight == bottomLeft) ? topLeft : 0.0f;
    }

    /**
     * Returns the uniform corner radius, or 0 if radii differ per corner.
     */
    inline float getCornerRadius() const { return this->m_cornerRadius; }

    /**
     * Sets the view shape shadow type.
     * Default is NONE.
     */
    inline void setShadowType(ShadowType type) { this->m_shadowType = type; }

    /**
     * Sets the shadow visibility.
     */
    inline void setShadowVisibility(bool visible) { this->m_showShadow = visible; }

    /**
     * If set to true, the highlight background will be hidden for this view
     * (the white rectangle that goes behind the view, replacing the usual
     * background shape).
     */
    inline void setHideHighlightBackground(bool hide) { this->m_hideHighlightBackground = hide; }

    /**
     * If set to true, the highlight border will be hidden for this view.
     */
    inline void setHideHighlightBorder(bool hide) { this->m_hideHighlightBorder = hide; }

    /**
     * If set to true, the highlight will be hidden for this view.
     */
    inline void setHideHighlight(bool hide) { this->m_hideHighlight = hide; }

    /**
     * If set to true, the click animation will be hidden for this view.
     */
    inline void setHideClickAnimation(bool hide) { this->m_hideClickAnimation = hide; }

    /**
     * Sets the highlight padding of the view, aka the space between the
     * highlight rectangle and the view. The highlight rect is enlarged, the view
     * is untouched.
     */
    inline void setHighlightPadding(float padding) { this->m_highlightPadding = padding; }

    /**
     * Sets the highlight rectangle corner radius.
     */
    inline void setHighlightCornerRadius(float radius) { this->m_highlightCornerRadius = radius; }

    // -----------------------------------------------------------

    /**
     * Returns the "nearest" view with the corresponding id, or nullptr if none
     * has been found. "Nearest" means the closest in the vicinity of this view.
     * The siblings are searched as well as its children.
     *
     * Research is done by traversing the tree upwards, starting from this view.
     * The current algorithm is very inefficient.
     */
    virtual View* getNearestView(const std::string& id);

    /**
     * Creates a view from the given XML file content.
     *
     * The method handleXMLElement() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLString(std::string_view xml);

    /**
     * Creates a view from the given XML element (node and attributes).
     *
     * The method handleXMLElement() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLElement(tinyxml2::XMLElement* element);

    /**
     * Creates a view from the given XML file path.
     *
     * The method handleXMLElement() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLFile(const std::string& path);

    /**
     * Creates a view from the given XML resource file name.
     *
     * The method handleXMLElement() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLResource(const std::string& name);

    /**
     * Handles a child XML element.
     *
     * You can redefine this method to handle child XML like
     * as you want in your own views.
     *
     * If left unimplemented, will throw an exception because raw
     * views cannot handle child XML elements (Boxes can).
     */
    virtual void handleXMLElement(tinyxml2::XMLElement* element);

    /**
     * Applies the attributes of the given XML element to the view.
     *
     * You can add your own attributes to by calling registerXMLAttribute()
     * in the view constructor.
     */
    virtual void applyXMLAttributes(tinyxml2::XMLElement* element);

    /**
     * Applies the given attribute to the view.
     *
     * You can add your own attributes to by calling registerXMLAttribute()
     * in the view constructor.
     */
    virtual bool applyXMLAttribute(const std::string& name, const std::string& value);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has the value "auto".
     */
    void registerAutoXMLAttribute(const std::string& name, const AutoAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has a percentage value (an
     * integer with "%" suffix). The given float value is guaranteed to be between
     * 0.0f and 1.0f.
     */
    void registerPercentageXMLAttribute(const std::string& name, const FloatAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has an integer, float, @style or
     * "px" value.
     */
    void registerFloatXMLAttribute(const std::string& name, const FloatAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has a string or @i18n value.
     *
     * If you use string as a type, you can only have one handler for the
     * attribute.
     */
    void registerStringXMLAttribute(const std::string& name, const StringAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has a color value ("#XXXXXX" or
     * "#XXXXXXXX") or a @theme value.
     */
    void registerColorXMLAttribute(const std::string& name, const ColorAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has a boolean value ("true" or
     * "false").
     */
    void registerBoolXMLAttribute(const std::string& name, const BoolAttributeHandler& handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is string.
     *
     * The method will be called if the attribute has a file path value ("@res/"
     * or raw path).
     */
    void registerFilePathXMLAttribute(const std::string& name, const FilePathAttributeHandler& handler);

    /**
     * Get the XML document for the XML file, creating a new one if none is
     * cached.
     */
    static std::shared_ptr<tinyxml2::XMLDocument> getXMLCache(std::string_view path);

    /**
     * Returns if the given XML attribute name is valid for that view.
     */
    bool isXMLAttributeValid(const std::string& attributeName);

    /**
     * Sets the maximum number of allowed children XML elements
     * when using a view of that class in an XML file.
     */
    void setMaximumAllowedXMLElements(unsigned max);

    /**
     * Returns the maximum number of allowed children XML elements.
     */
    unsigned getMaximumAllowedXMLElements() const;

    /**
     * If set to true, will force the view to be translucent.
     */
    void setInFadeAnimation(bool translucent);

    /**
     * Sets the view to be focusable.
     *
     * Required to be able to use actions that need
     * focus on that view (such as an A press).
     */
    inline void setFocusable(bool focusable) { this->m_focusable = focusable; }

    /**
     * Returns whether this view is focusable and visible.
     */
    bool isFocusable();

    /**
     * Removes view from it's parent
     */
    void removeFromSuperView(bool free = true);

    /**
     * Sets the sound to play when this view gets focused.
     */
    inline void setFocusSound(enum Sound sound) { this->m_focusSound = sound; }

    /**
     * Returns the sound to play when this view gets focused.
     */
    virtual enum Sound getFocusSound();

    /**
     * Sets the detached flag to true.
     * This action is irreversible.
     *
     * A detached view will, as the name suggests, not be
     * attached to their parent Yoga node. That means that invalidation
     * and layout need to be taken care of manually by the parent.
     *
     * detach() must be called before adding the view to the parent.
     */
    void detach();

    /**
     * Returns whether this view is detached from its parent's layout.
     */
    bool isDetached() const;

    /**
     * Sets the position of the view, if detached.
     */
    void setDetachedPosition(float x, float y);

    /**
     * Sets the position X of the view, if detached.
     */
    void setDetachedPositionX(float x);

    /**
     * Sets the position Y of the view, if detached.
     */
    void setDetachedPositionY(float y);

    /**
     * Gets detached position of the view.
     */
    Point getDetachedPosition() const { return m_detachedOrigin; }

    /**
     * Sets the parent box of this view and optional user data.
     */
    void setParent(Box* parent, void* parentUserdata = nullptr);

    /**
     * Returns the parent box of this view, or nullptr if none.
     */
    Box* getParent();

    /**
     * Returns whether this view has a parent.
     */
    bool hasParent();

    /**
     * Returns the parent user data pointer associated with this view.
     */
    void* getParentUserData();

    /**
     * Registers an action with the given parameters. The listener will be fired
     * when the user presses the key when the view is focused.
     *
     * The listener should return true if the action was consumed, false
     * otherwise. The sound will only be played if the listener returned true.
     *
     * A hidden action will not show up in the bottom-right hints.
     *
     * Returns the identifier for the action, so it can be unregistered later on.
     * Returns ACTION_NONE if the action was not registered.
     */
    ActionIdentifier registerAction(
        const std::string& hintText,
        enum ControllerButton button,
        const ActionListener& actionListener,
        bool hidden         = false,
        bool allowRepeating = false,
        enum Sound sound    = SOUND_NONE
    );

    /**
     * Unregisters an action with the given identifier.
     */
    void unregisterAction(ActionIdentifier identifier);

    /**
     * Shortcut to register a generic "A OK" click action.
     */
    void registerClickAction(const ActionListener& actionListener);

    /**
     * Updates the hint text for the action bound to the given button.
     */
    void updateActionHint(enum ControllerButton button, const std::string& hintText);

    /**
     * Sets whether the action bound to the given button is available.
     */
    void setActionAvailable(enum ControllerButton button, bool available);

    /**
     * Sets whether all actions on this view are available.
     */
    void setActionsAvailable(bool available);

    /**
     * Stops any currently running click animation.
     */
    void resetClickAnimation();

    /**
     * Plays the click feedback animation on this view.
     * @param reverse if true, plays the animation in reverse.
     * @param animateBack if true, automatically plays the reverse after completing.
     * @param force if true, plays even if click animation is hidden.
     */
    void playClickAnimation(bool reverse = false, bool animateBack = true, bool force = false);

    /**
     * Returns the demangled class name of this view.
     */
    std::string getClassString() const
    {
        // Taken from:
        // https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname/4541470#4541470
        const char* name = typeid(*this).name();
#ifndef _MSC_VER
        int status = 0;
        std::unique_ptr<char, void (*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
        return (status == 0) ? res.get() : name;
#else
        return name;
#endif
    }

    /**
     * Returns a human-readable description of this view (class name and id).
     */
    std::string describe() const
    {
        std::string classString = this->getClassString();

        if (this->id != "")
            return classString + " (id=\"" + this->id + "\")";

        return classString;
    }

    /**
     * Returns the underlying Yoga layout node for this view.
     */
    YGNode* getYGNode() { return this->ygNode; }

    /**
     * Returns the vector of all actions registered on this view.
     */
    const std::vector<Action>& getActions() { return this->m_actions; }

    /**
     * Get the vector of all gesture recognizers attached to that view.
     */
    const std::vector<GestureRecognizer*>& getGestureRecognizers() { return this->m_gestureRecognizers; }

    /**
     * Interrupt every recognizer on this view.
     * If onlyIfUnsureState == true, only recognizers with
     * current state UNSURE will be interupted
     */
    void interruptGestures(bool onlyIfUnsureState);

    /**
     * Add new gesture recognizer on this view.
     */
    void addGestureRecognizer(GestureRecognizer* recognizer);

    /**
     * Remove a gesture recognizer from this view.
     */
    void removeGestureRecognizer(GestureRecognizer* recognizer);

    /**
     * Called each frame when touch is registered.
     *
     * @returns sound to play invoked by touch recognizers.
     */
    Sound gestureRecognizerRequest(TouchState touch, MouseState mouse, View* firstResponder);

    /**
     * Called each frame
     * Do not override it to draw your view,
     * override draw() instead
     */
    virtual void frame(FrameContext* ctx);

    /**
     * Called each frame
     */
    void frameHighlight(FrameContext* ctx);

    /**
     * Called by frame() to draw the view onscreen.
     * Views should not draw outside of their bounds (they
     * may be clipped if they do so).
     */
    virtual void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) = 0;

    /**
     * Called when the view will appear
     * on screen, before or after layout().
     *
     * Can be called if the view has
     * already appeared, so be careful.
     */
    virtual void willAppear(bool resetState = false)
    {
        // Nothing to do
    }

    /**
     * Called when the view will disappear
     * from the screen.
     *
     * Can be called if the view has
     * already disappeared, so be careful.
     */
    virtual void willDisappear(bool resetState = false)
    {
        // Nothing to do
    }

    /**
     * Called when the show() animation (fade in)
     * ends
     */
    virtual void onShowAnimationEnd() {};

    /**
     * Shows the view with a fade in animation.
     */
    virtual void show(const std::function<void(void)>& cb);

    /**
     * Shows the view with a fade in animation, or no animation at all.
     */
    virtual void show(const std::function<void(void)>& cb, bool animate, float animationDuration);

    /**
     * Returns the duration of the view show / hide animation.
     */
    virtual float getShowAnimationDuration(TransitionAnimation animation);

    /**
     * Hides the view in a collapse animation
     */
    void collapse(bool animated = true);

    /**
     * Returns whether this view is currently collapsed or collapsing.
     */
    bool isCollapsed();

    /**
     * Sets the opacity of this view directly (without animation).
     */
    void setAlpha(float alpha);

    /**
     * Shows the view in a expand animation (opposite
     * of collapse)
     */
    void expand(bool animated = true);

    /**
     * Hides the view with a fade out animation.
     */
    virtual void hide(const std::function<void(void)>& cb);

    /**
     * Hides the view with a fade out animation, or no animation at all.
     */
    virtual void hide(const std::function<void(void)>& cb, bool animate, float animationDuration);

    /**
     * Returns whether this view is currently hidden.
     */
    bool isHidden() const;

    /**
     * Is this view translucent?
     *
     * If you override it please return
     * <value> || View::isTranslucent()
     * to keep the fadeIn transition
     */
    virtual bool isTranslucent();

    /**
     * Returns whether this view currently has focus.
     */
    bool isFocused() const;

    /**
     * Returns the default view to focus when focusing this view
     * Typically the view itself or one of its children.
     *
     * Returning nullptr means that the view is not focusable
     * (and neither are its children)
     *
     * By default, a view is focusable if the flag is set to true with
     * setFocusable() and if the view is visible.
     *
     * When pressing a key, the flow is :
     *    1. starting from the currently focused view's parent, traverse the tree
     * upwards and repeatedly call getNextFocus() on every view until we find a
     * next view to focus or meet the end of the tree
     *    2. if a view is found, getNextFocus() will internally call
     * getDefaultFocus() for the selected child
     *    3. give focus to the result, if it exists
     */
    virtual View* getDefaultFocus();

    /**
     * Returns the view to focus with the corresponding screen coordinates in the
     * view or its children, or nullptr if it hasn't been found.
     *
     * Research is done recursively by traversing the tree starting from this
     * view. This view's parents are not traversed.
     */
    virtual View* hitTest(Point point);

    /**
     * Returns the next view to focus given the requested direction
     * and the currently focused view (as parent user data)
     *
     * Returning nullptr means that there is no next view to focus
     * in that direction - getNextFocus will then be called on our
     * parent if any
     */
    virtual View* getNextFocus(FocusDirection direction, View* currentView);

    /**
     * Sets a custom navigation route from this view to the target one.
     */
    void setCustomNavigationRoute(FocusDirection direction, View* target);

    /**
     * Sets a custom navigation route from this view to the target one, by ID.
     * The final target view will be the "nearest" with the given ID.
     *
     * Resolution of the ID to View is made when the navigation event occurs, not
     * when the route is registered.
     */
    void setCustomNavigationRoute(FocusDirection direction, const std::string& targetId);

    /**
     * Sets a custom navigation route for all direction from this view to the
     * target one, by ID. The final target view will be the "nearest" with the given ID.
     *
     * Resolution of the ID to View is made when the navigation event occurs, not
     * when the route is registered.
     */
    void setCustomNavigationRoute(const std::string& targetId);

    /**
     * Returns whether a custom navigation route by pointer exists for the given direction.
     */
    bool hasCustomNavigationRouteByPtr(FocusDirection direction);

    /**
     * Returns whether a custom navigation route by ID exists for the given direction.
     */
    bool hasCustomNavigationRouteById(FocusDirection direction);

    /**
     * Returns the custom navigation route target view for the given direction.
     */
    View* getCustomNavigationRoutePtr(FocusDirection direction);

    /**
     * Returns the custom navigation route target ID for the given direction.
     */
    std::string getCustomNavigationRouteId(FocusDirection direction);

    /**
     * Fired when focus is gained.
     */
    virtual void onFocusGained();

    /**
     * Fired when focus is lost.
     */
    virtual void onFocusLost();

    /**
     * Fired when focus is gained on this view's parent, or the parent of the
     * parent...
     */
    virtual void onParentFocusGained(View* focusedView);

    /**
     * Fired when focus is lost on one of this view's parents. Works similarly to
     * onParentFocusGained().
     */
    virtual void onParentFocusLost(View* focusedView);

    /**
     * Fired when the window size changes, after updating
     * layout.
     */
    virtual void onWindowSizeChanged()
    {
        // Nothing by default
    }

    /**
     * Returns the event fired when this view gains focus.
     */
    GenericEvent* getFocusEvent();

    /**
     * Returns the event fired when this view loses focus.
     */
    GenericEvent* getFocusLostEvent();

    /**
     * The opacity of this view. Animatable between 0.0f and 1.0f.
     */
    Animatable alpha = 1.0f;

    /**
     * Returns the effective alpha of the view, multiplied by parent alpha.
     * @param child used internally to distinguish parent alpha queries.
     */
    virtual float getAlpha(bool child = false);

    /**
     * Returns the current click animation alpha value.
     */
    float getClickAlpha() { return this->m_clickAlpha; }

    /**
     * Sets the click animation alpha value directly.
     */
    void setClickAlpha(float a) { this->m_clickAlpha = a; }

    /**
     * Forces this view and its children to use
     * the specified theme.
     */
    void overrideTheme(Theme* newTheme);

    /**
     * Enables / disable culling for that view.
     *
     * To disable culling for all child views
     * of a Box use setCullingEnabled on the box.
     */
    void setCulled(bool culled) { this->m_culled = culled; }

    /**
     * Returns whether this view is currently culled (not drawn by its parent).
     */
    bool isCulled() const { return this->m_culled; }

    /**
     * Sets the aspect ratio constraint for this view.
     * The value represents width / height.
     */
    void setAspectRatio(float value)
    {
        if (value <= 0)
            return;
        this->m_aspectRatio = value;
        YGNodeStyleSetAspectRatio(this->ygNode, value);
        this->invalidate();
    }

    /**
     * Returns the aspect ratio constraint of this view, or 0 if not set.
     */
    float getAspectRatio() const { return this->m_aspectRatio; }

    /**
     * Sets the background corner radii of the view. Only for vertical linear
     * style.
     */
    void setBackgroundCornerRadii(float topLeft, float topRight, float bottomRight, float bottomLeft)
    {
        this->m_backgroundRadius = { topLeft, topRight, bottomRight, bottomLeft };
    }

    /**
     * Sets the Y translation of this view.
     *
     * Translation is applied after the layout phase. Use relative position
     * and setPosition methods if possible instead.
     */
    void setTranslationY(float translationY);

    /**
     * Sets the X translation of this view.
     *
     * Translation is applied after the layout phase. Use relative position
     * and setPosition methods if possible instead.
     */
    void setTranslationX(float translationX);

    /**
     * Sets the horizontal scale of this view.
     *
     * Use setScale to apply uniform scaling on both axes instead.
     */
    void setScaleX(float scaleX);

    /**
     * Sets the vertical scale of this view.
     *
     * Use setScale to apply uniform scaling on both axes instead.
     */
    void setScaleY(float scaleY);

    /**
     * Sets a uniform scale factor for both the X and Y axes of this view.
     *
     * This is a convenience method equivalent to calling setScaleX and
     * setScaleY with the same value.
     */
    void setScale(float scale);

    /**
     * Gets the current scale of this view as a Point.
     *
     * Returns a Point containing the X and Y scale factors.
     */
    Point getScale();

    /**
     * Wireframe mode allows you to see the view size and margins (and
     * padding if applicable) directly in your app.
     *
     * Useful to diagnose views misplacements or display bugs.
     */
    void setWireframeEnabled(bool wireframe);

    /**
     * Returns whether wireframe mode is enabled for this view.
     */
    bool isWireframeEnabled() const;

    /**
     * Resolves the value of the given XML attribute string.
     * Applies i18n if the value is an "@i18n/" string, returns the
     * string as it is otherwise.
     */
    static std::string getStringXMLAttributeValue(std::string value);

    /**
     * Resolves the value of the given XML attribute file path.
     * Returns the full path of the resource if it starts with "@res/", returns
     * the path as it is otherwise.
     */
    static std::string getFilePathXMLAttributeValue(std::string value);

    /**
     * Returns the applet frame item metadata (title, icon) for this view.
     */
    AppletFrameItem* getAppletFrameItem() { return &this->m_appletFrameItem; }

    /**
     * Updates the applet frame with this view's title and icon metadata.
     */
    void updateAppletFrameItem();

    /**
     * Returns whether this view clips its children to its bounds.
     */
    bool getClipsToBounds() const { return m_clipsToBounds; }

    /**
     * Sets whether this view clips its children to its bounds.
     */
    void setClipsToBounds(bool value) { m_clipsToBounds = value; }

    /**
     * Returns the nearest AppletFrame ancestor of this view, or nullptr.
     */
    virtual AppletFrame* getAppletFrame();

    /**
     * Pushes a view onto the nearest AppletFrame's content stack.
     */
    void present(View* view);

    /**
     * Pops the current view from the nearest AppletFrame's content stack.
     */
    virtual void dismiss(std::function<void(void)> cb = [] {});

    bool* deletionToken       = nullptr;
    int* deletionTokenCounter = nullptr;

    /**
     * Increments the pointer lock counter, preventing deletion.
     */
    void ptrLock();

    /**
     * Decrements the pointer lock counter.
     */
    void ptrUnlock();

    /**
     * Returns whether this view is pointer-locked (deletion deferred).
     */
    bool isPtrLocked() const;

    /**
     * Enqueues this view for deferred deletion.
     */
    void freeView();

    /**
     * Returns the Activity that owns this view, traversing parents if needed.
     */
    Activity* getParentActivity();

    /**
     * Sets the Activity that owns this view.
     */
    void setParentActivity(Activity* activity);
};

} // namespace brls
