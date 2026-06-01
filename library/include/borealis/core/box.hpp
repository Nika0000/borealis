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

#include <borealis/core/view.hpp>

namespace brls
{

/** Controls how children are distributed along the main axis of a Box. */
enum class JustifyContent
{
    FLEX_START,
    CENTER,
    FLEX_END,
    SPACE_BETWEEN,
    SPACE_AROUND,
    SPACE_EVENLY,
};

/** Controls how children are aligned along the cross axis of a Box. */
enum class AlignItems
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

/** The main layout direction of a Box. */
enum class Axis
{
    ROW,
    COLUMN,
};

/** Controls whether children wrap to new lines when they overflow the main axis. */
enum class Wrap
{
    NO_WRAP,
    WRAP,
    WRAP_REVERSE,
};

/** Controls the text/layout direction of a Box. */
enum class Direction
{
    INHERIT,
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
};

/**
 * Generic FlexBox layout container.
 *
 * A Box is a view that can contain children views arranged along a given axis
 * (row or column), following the CSS Flexbox layout model powered by Yoga.
 *
 * Children can be justified and aligned using setJustifyContent() and setAlignItems().
 * The box supports padding, gap, wrapping, and direction control.
 */
class Box : public View
{
  public:
    /**
     * Creates a Box with the given flex direction axis.
     */
    Box(Axis flexDirection);

    /**
     * Creates a Box with the default axis (ROW).
     */
    Box();
    ~Box();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    View* getDefaultFocus() override;
    View* hitTest(Point point) override;
    View* getNextFocus(FocusDirection direction, View* currentView) override;
    void willAppear(bool resetState) override;
    void willDisappear(bool resetState) override;
    void onWindowSizeChanged() override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onParentFocusGained(View* focusedView) override;
    void onParentFocusLost(View* focusedView) override;
    bool applyXMLAttribute(const std::string& name, const std::string& value) override;

    /** Creates a new Box instance for use with the XML view factory. */
    static View* create();

    /**
     * Called when a child requests focus navigation. Allows the box to override
     * or redirect the focus decision.
     */
    virtual View* getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction);

    /**
     * Adds a view to this Box.
     * Returns the position the view was added at.
     */
    virtual void addView(View* view);

    /**
     * Adds a view to this Box at the given position.
     * Returns the position the view was added at.
     */
    virtual void addView(View* view, size_t position);

    /**
     * Removes the given view from the Box. It will be freed.
     */
    virtual void removeView(View* view, bool free = true);

    /**
     * Removes all views from the Box. Them will be freed.
     */
    virtual void clearViews(bool free = true);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    virtual void setPadding(float padding);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    virtual void setPadding(float top, float right, float bottom, float left);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingTop(float top);

    /** Returns the top padding value. */
    float getPaddingTop();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingRight(float right);

    /** Returns the right padding value. */
    float getPaddingRight();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingBottom(float bottom);

    /** Returns the bottom padding value. */
    float getPaddingBottom();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingLeft(float left);

    /** Returns the left padding value. */
    float getPaddingLeft();

    /**
     * Sets the gap between child elements inside this view.
     * This is typically used in layouts like grids or lists.
     */
    virtual void setGap(float gap);

    /**
     * Returns the current gap between child elements inside this view.
     */
    float getGap();

    /**
     * Sets the children alignment along the Box axis.
     *
     * Default is FLEX_START.
     */
    void setJustifyContent(JustifyContent justify);

    /**
     * Sets the children alignment along the Box cross axis.
     *
     * Default is AUTO.
     */
    void setAlignItems(AlignItems alignment);

    /**
     * Sets the direction of the box, aka place the views
     * left to right or right to left (flips the children).
     *
     * Default is INHERIT.
     */
    void setDirection(Direction direction);

    /** Sets the main axis of the box layout. */
    void setAxis(Axis axis);

    /** Returns the current main axis of the box layout. */
    Axis getAxis() const;

    /**
     * Sets the wrapping behavior for the children inside this view.
     */
    void setWrap(Wrap wrap);

    /**
     * Returns the wrapping behavior for the children inside this view.
     */
    Wrap getWrap() const;

    /** Returns a reference to the vector of child views. */
    std::vector<View*>& getChildren();

    /**
     * Returns the bounds used for culling children.
     */
    virtual void getCullingBounds(float* top, float* right, float* bottom, float* left);

    /**
     * Registers an XML attribute to be forwarded to the given view. Works regardless of the target attribute type.
     * Useful to expose attributes of children views in the parent box without copy pasting them individually.
     *
     * The forwarded attribute value will override the value of the regular attribute if it already exists in the target view.
     */
    void forwardXMLAttribute(const std::string& attributeName, View* target);

    /**
     * Registers an XML attribute to be forwarded to the given view, while changing the target attribute name.
     * Works regardless of the target attribute type.
     * Useful to expose attributes of children views in the parent box without copy pasting them individually, but with a different name.
     *
     * The forwarded attribute value will override the value of the regular attribute if it already exists in the target view.
     */
    void forwardXMLAttribute(const std::string& attributeName, View* target, const std::string& targetAttributeName);

    /**
     * Fired when focus is gained on one of this view's children, or one of the children
     * of the children...
     *
     * directChild is guaranteed to be one of your children. It may not be the view that has been
     * focused.
     *
     * If focusedView == directChild, then the child of yours has been focused.
     * Otherwise, focusedView is a child of directChild.
     */
    virtual void onChildFocusGained(View* directChild, View* focusedView);

    /**
     * Fired when focus is lost on one of this view's children. Works similarly to
     * onChildFocusGained().
     */
    virtual void onChildFocusLost(View* directChild, View* focusedView);

    View* getView(const std::string& id) override;

    /** Sets the last focused child view, used for focus memory. */
    void setLastFocusedView(View* view);

    /** Sets the default child index to focus when this box receives focus. */
    void setDefaultFocusedIndex(int index);

    /** Returns the default child index to focus. */
    size_t getDefaultFocusedIndex() const;

    /** Returns the last child view that held focus inside this box. */
    View* getLastFocusedView() const;

    /** Returns true if any child (or descendant) currently holds focus. */
    virtual bool isChildFocused();

  private:
    Axis m_axis;

    std::vector<View*> m_children;

    size_t m_defaultFocusedIndex = 0;
    View* m_lastFocusedView      = nullptr;

    std::unordered_map<std::string, std::pair<std::string, View*>> m_forwardedAttributes;

  protected:
    /**
     * Inflates the Box with the given XML string.
     *
     * The root element MUST be a brls::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root brls::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLString(std::string_view xml);

    /**
     * Inflates the Box with the given XML element.
     *
     * The root element MUST be a brls::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root brls::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLElement(tinyxml2::XMLElement* element);

    /**
     * Inflates the Box with the given XML resource.
     *
     * The root element MUST be a brls::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root brls::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLRes(const std::string& res);

    /**
     * Inflates the Box with the given XML file path.
     *
     * The root element MUST be a brls::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root brls::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLFile(const std::string& path);

    /**
     * Handles a child XML element.
     *
     * By default, calls createFromXMLElement() and adds the result
     * to the children of the Box.
     */
    void handleXMLElement(tinyxml2::XMLElement* element) override;
};

/**
 * An empty spacer view with auto size and grow=1.0.
 *
 * When placed in a Box, it pushes all subsequent siblings to the opposite end
 * of the axis (e.g., to the right in a row, or to the bottom in a column).
 */
class Padding : public View
{
  public:
    Padding();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    /** Creates a new Padding instance for use with the XML view factory. */
    static View* create();
};

} // namespace brls
