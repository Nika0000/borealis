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

#include <tinyxml2.h>

#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/core/util.hpp>
#include <cmath>
#include <fstream>
#include <limits>

namespace brls
{

static YGFlexDirection getYGFlexDirection(Axis axis)
{
    switch (axis)
    {
        default:
        case Axis::ROW:
            return YGFlexDirectionRow;
        case Axis::COLUMN:
            return YGFlexDirectionColumn;
    }
}

static YGWrap getYGWrap(Wrap wrap)
{
    switch (wrap)
    {
        case Wrap::WRAP:
            return YGWrapWrap;
        case Wrap::WRAP_REVERSE:
            return YGWrapWrapReverse;
        default:
            return YGWrapNoWrap;
    }
}

Box::Box(Axis flexDirection) : m_axis(flexDirection)
{
    YGNodeStyleSetFlexDirection(this->ygNode, getYGFlexDirection(flexDirection));

    // no need to invalidate if the box is empty and is not attached to any parent

    // Register XML attributes
    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "axis",
        Axis,
        this->setAxis,
        {
            { "row", Axis::ROW },
            { "column", Axis::COLUMN },
        }
    );

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "flexWrap",
        Wrap,
        this->setWrap,
        {
            { "wrap", Wrap::WRAP },
            { "noWrap", Wrap::NO_WRAP },
            { "wrapReverse", Wrap::WRAP_REVERSE },
        }
    );

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "direction",
        Direction,
        this->setDirection,
        {
            { "inherit", Direction::INHERIT },
            { "leftToRight", Direction::LEFT_TO_RIGHT },
            { "rightToLeft", Direction::RIGHT_TO_LEFT },
        }
    );

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "justifyContent",
        JustifyContent,
        this->setJustifyContent,
        {
            { "flexStart", JustifyContent::FLEX_START },
            { "center", JustifyContent::CENTER },
            { "flexEnd", JustifyContent::FLEX_END },
            { "spaceBetween", JustifyContent::SPACE_BETWEEN },
            { "spaceAround", JustifyContent::SPACE_AROUND },
            { "spaceEvenly", JustifyContent::SPACE_EVENLY },
        }
    );

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "alignItems",
        AlignItems,
        this->setAlignItems,
        {
            { "auto", AlignItems::AUTO },
            { "flexStart", AlignItems::FLEX_START },
            { "center", AlignItems::CENTER },
            { "flexEnd", AlignItems::FLEX_END },
            { "stretch", AlignItems::STRETCH },
            { "baseline", AlignItems::BASELINE },
            { "spaceBetween", AlignItems::SPACE_BETWEEN },
            { "spaceAround", AlignItems::SPACE_AROUND },
        }
    );

    // Gap
    this->registerFloatXMLAttribute("gap", [this](float value) { this->setGap(value); });

    // Padding
    this->registerFloatXMLAttribute("paddingTop", [this](float value) { this->setPaddingTop(value); });
    this->registerFloatXMLAttribute("paddingRight", [this](float value) { this->setPaddingRight(value); });
    this->registerFloatXMLAttribute("paddingBottom", [this](float value) { this->setPaddingBottom(value); });
    this->registerFloatXMLAttribute("paddingLeft", [this](float value) { this->setPaddingLeft(value); });
    this->registerFloatXMLAttribute("padding", [this](float value) { this->setPadding(value); });
}

Box::Box() : Box(Axis::ROW)
{
    // Empty ctor for XML
}

void Box::getCullingBounds(float* top, float* right, float* bottom, float* left)
{
    *top    = this->getY();
    *left   = this->getX();
    *right  = *left + this->getWidth();
    *bottom = *top + this->getHeight();
}

void Box::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    for (View* child : this->m_children)
    {
        // Ensure that the child is in bounds of all parents before drawing it
        // Only do that check for leaf views, as nested boxes will do that check themselves
        if (!dynamic_cast<Box*>(child) && child->isCulled())
        {
            float childTop    = child->getY();
            float childLeft   = child->getX();
            float childRight  = childLeft + child->getWidth();
            float childBottom = childTop + child->getHeight();

            bool draw = true;

            for (Box* bounds = this; bounds != nullptr; bounds = bounds->getParent())
            {
                float top, right, bottom, left;
                bounds->getCullingBounds(&top, &right, &bottom, &left);

                if (
                    childBottom < top || // too high
                    childRight < left || // too far left
                    childLeft > right || // too far right
                    childTop > bottom    // too low
                )
                {
                    draw = false;
                    break;
                }
            }

            if (!draw)
                continue;
        }

        child->frame(ctx);
    }
}

void Box::addView(View* view)
{
    size_t position = YGNodeGetChildCount(this->ygNode);
    this->addView(view, position);
}

void Box::addView(View* view, size_t position)
{
    if (position > this->m_children.size() || position < 0)
        fatal(fmt::format("cannot insert view at {}:{}/{}", this->describe(), this->m_children.size(), position));

    // Add the view to our children and YGNode
    this->m_children.insert(this->m_children.begin() + position, view);

    if (!view->isDetached())
        YGNodeInsertChild(this->ygNode, view->getYGNode(), position);

    // Allocate and set parent userdata
    size_t* userdata = (size_t*) malloc(sizeof(size_t));
    *userdata        = position;

    view->setParent(this, userdata);

    for (size_t i = position + 1; i < this->m_children.size(); i++)
    {
        auto* index = (size_t*) this->m_children[i]->getParentUserData();
        if (index)
            (*index)++;
    }

    // Layout and events
    this->invalidate();
    view->willAppear();
}

void Box::removeView(View* view, bool free)
{
    if (!view)
        return;

    // Find the index of the view
    size_t index;
    bool found = false;

    for (size_t i = 0; i < this->m_children.size(); i++)
    {
        View* child = this->m_children[i];

        if (child == view)
        {
            found = true;
            index = i;
            break;
        }
    }

    if (!found)
        return;

    // Remove it
    if (!view->isDetached())
        YGNodeRemoveChild(this->ygNode, view->getYGNode());
    this->m_children.erase(this->m_children.begin() + index);

    // Update parent userdata
    for (size_t i = index; i < this->m_children.size(); i++)
    {
        auto* index = (size_t*) this->m_children[i]->getParentUserData();
        if (index)
            (*index)--;
    }

    view->willDisappear(true);
    if (free)
        view->freeView();

    this->invalidate();
}

void Box::clearViews(bool free)
{
    m_lastFocusedView        = nullptr;
    std::vector<View*> views = getChildren();

    for (size_t i = 0; i < views.size(); i++)
    {
        View* view = this->m_children.back();

        // Remove it
        YGNodeRemoveChild(this->ygNode, view->getYGNode());
        this->m_children.pop_back();

        view->willDisappear(true);
        if (free)
            view->freeView();
    }

    this->invalidate();
}

void Box::onFocusGained()
{
    View::onFocusGained();

    for (View* child : this->m_children)
        child->onParentFocusGained(this);
}

void Box::onFocusLost()
{
    View::onFocusLost();

    for (View* child : this->m_children)
        child->onParentFocusLost(this);
}

void Box::onParentFocusGained(View* focusedView)
{
    View::onParentFocusGained(focusedView);

    for (View* child : this->m_children)
        child->onParentFocusGained(focusedView);
}

void Box::onParentFocusLost(View* focusedView)
{
    View::onParentFocusLost(focusedView);

    for (View* child : this->m_children)
        child->onParentFocusLost(focusedView);
}

void Box::setPadding(float top, float right, float bottom, float left)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeTop, top);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeRight, right);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeBottom, bottom);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void Box::setPadding(float padding) { this->setPadding(padding, padding, padding, padding); }

void Box::setPaddingTop(float top)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeTop, top);
    this->invalidate();
}

void Box::setPaddingRight(float right)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeRight, right);
    this->invalidate();
}

void Box::setPaddingBottom(float bottom)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeBottom, bottom);
    this->invalidate();
}

void Box::setPaddingLeft(float left)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeLeft, left);
    this->invalidate();
}

float Box::getPaddingTop() { return YGNodeStyleGetPadding(this->ygNode, YGEdgeTop).value; }

float Box::getPaddingBottom() { return YGNodeStyleGetPadding(this->ygNode, YGEdgeBottom).value; }

float Box::getPaddingLeft() { return YGNodeStyleGetPadding(this->ygNode, YGEdgeLeft).value; }
float Box::getPaddingRight() { return YGNodeStyleGetPadding(this->ygNode, YGEdgeRight).value; }

void Box::setGap(float gap)
{
    YGNodeStyleSetGap(this->ygNode, YGGutterAll, gap);
    this->invalidate();
}

float Box::getGap() { return YGNodeStyleGetGap(this->ygNode, YGGutterAll); }

View* Box::getDefaultFocus()
{
    if (this->getVisibility() != Visibility::VISIBLE)
        return nullptr;

    // Focus ourself first
    if (this->isFocusable())
        return this;

    if (m_lastFocusedView)
    {
        View* view = m_lastFocusedView->getDefaultFocus();
        if (view)
            return view;
    }

    // Then try default focus
    if (this->m_defaultFocusedIndex < this->m_children.size())
    {
        View* newFocus = this->m_children[this->m_defaultFocusedIndex]->getDefaultFocus();

        if (newFocus)
            return newFocus;
    }

    // Fallback to finding the first focusable view
    for (size_t i = 0; i < this->m_children.size(); i++)
    {
        View* newFocus = this->m_children[i]->getDefaultFocus();

        if (newFocus)
            return newFocus;
    }

    return nullptr;
}

View* Box::hitTest(Point point)
{
    // Check if can focus fearther first
    if (alpha == 0.0f || getVisibility() != Visibility::VISIBLE)
        return nullptr;

    // Check if touch fits in view frame
    if (this->getFrame().pointInside(point))
    {
        for (auto child = this->m_children.rbegin(); child != this->m_children.rend(); child++)
        {
            View* result = (*child)->hitTest(point);

            if (result)
                return result;
        }

        return this;
    }

    return nullptr;
}

View* Box::getNextFocus(FocusDirection direction, View* currentView)
{
    void* parentUserData = currentView->getParentUserData();

    // For wrapped layouts, use spatial navigation for the cross-axis direction
    bool isWrapped   = YGNodeStyleGetFlexWrap(this->ygNode) != YGWrapNoWrap;
    bool isCrossAxis = (this->m_axis == Axis::ROW && (direction == FocusDirection::UP || direction == FocusDirection::DOWN))
                       || (this->m_axis == Axis::COLUMN && (direction == FocusDirection::LEFT || direction == FocusDirection::RIGHT));

    if (isCrossAxis && isWrapped && !this->m_children.empty())
    {
        Rect currentFrame    = currentView->getFrame();
        float currentCenterX = currentFrame.getMinX() + currentFrame.getWidth() / 2.0f;
        float currentCenterY = currentFrame.getMinY() + currentFrame.getHeight() / 2.0f;

        View* bestCandidate = nullptr;
        float bestDistance  = std::numeric_limits<float>::max();

        for (View* child : this->m_children)
        {
            if (child == currentView)
                continue;

            View* focusable = child->getDefaultFocus();
            if (!focusable)
                continue;

            Rect frame    = child->getFrame();
            float centerX = frame.getMinX() + frame.getWidth() / 2.0f;
            float centerY = frame.getMinY() + frame.getHeight() / 2.0f;

            bool valid = false;
            switch (direction)
            {
                case FocusDirection::UP:
                    valid = centerY < currentCenterY;
                    break;
                case FocusDirection::DOWN:
                    valid = centerY > currentCenterY;
                    break;
                case FocusDirection::LEFT:
                    valid = centerX < currentCenterX;
                    break;
                case FocusDirection::RIGHT:
                    valid = centerX > currentCenterX;
                    break;
            }

            if (!valid)
                continue;

            // Prefer closest on the cross axis, then closest on main axis
            float crossDist, mainDist;
            if (direction == FocusDirection::UP || direction == FocusDirection::DOWN)
            {
                crossDist = std::abs(centerY - currentCenterY);
                mainDist  = std::abs(centerX - currentCenterX);
            }
            else
            {
                crossDist = std::abs(centerX - currentCenterX);
                mainDist  = std::abs(centerY - currentCenterY);
            }

            float distance = crossDist * 1000.0f + mainDist;
            if (distance < bestDistance)
            {
                bestDistance  = distance;
                bestCandidate = focusable;
            }
        }

        View* result = getParentNavigationDecision(this, bestCandidate, direction);
        if (!result && hasParent())
            result = getParent()->getNextFocus(direction, this);
        return result;
    }

    // Return nullptr immediately if focus direction mismatches the box axis
    if ((this->m_axis == Axis::ROW && direction != FocusDirection::LEFT && direction != FocusDirection::RIGHT)
        || (this->m_axis == Axis::COLUMN && direction != FocusDirection::UP && direction != FocusDirection::DOWN))
    {
        View* next = getParentNavigationDecision(this, nullptr, direction);
        if (!next && hasParent())
            next = getParent()->getNextFocus(direction, this);
        return next;
    }

    // Traverse the children
    size_t offset = 1; // which way we are going in the children list

    if ((this->m_axis == Axis::ROW && direction == FocusDirection::LEFT)
        || (this->m_axis == Axis::COLUMN && direction == FocusDirection::UP))
    {
        offset = -1;
    }

    size_t currentFocusIndex = *((size_t*) parentUserData) + offset;
    View* currentFocus       = nullptr;

    while (!currentFocus && currentFocusIndex >= 0 && currentFocusIndex < this->m_children.size())
    {
        currentFocus = this->m_children[currentFocusIndex]->getDefaultFocus();
        currentFocusIndex += offset;
    }

    currentFocus = getParentNavigationDecision(this, currentFocus, direction);
    if (!currentFocus && hasParent())
        currentFocus = getParent()->getNextFocus(direction, this);
    return currentFocus;
}

View* Box::getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction)
{
    if (!hasParent())
        return newFocus;

    return getParent()->getParentNavigationDecision(from, newFocus, direction);
}

void Box::willAppear(bool resetState)
{
    for (View* child : this->m_children)
        child->willAppear(resetState);
}

void Box::willDisappear(bool resetState)
{
    for (View* child : this->m_children)
        child->willDisappear(resetState);
}

void Box::onWindowSizeChanged()
{
    for (View* child : this->m_children)
        child->onWindowSizeChanged();
}

std::vector<View*>& Box::getChildren() { return this->m_children; }

void Box::inflateFromXMLString(std::string_view xml)
{
    // Load XML
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(xml);
    tinyxml2::XMLElement* element                   = document->RootElement();

    if (!element)
    {
        tinyxml2::XMLError error = document->Parse(xml.data());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Invalid XML when inflating " + this->describe() + ": error " + std::to_string(error));

        element = document->RootElement();

        if (!element)
            fatal("Invalid XML: no element found");
    }

    return Box::inflateFromXMLElement(element);
}

void Box::inflateFromXMLRes(const std::string& res)
{
    // Check if custom xml file exists
    if (!View::CUSTOM_RESOURCES_PATH.empty() && std::ifstream { View::CUSTOM_RESOURCES_PATH + res }.good())
    {
        return Box::inflateFromXMLFile(View::CUSTOM_RESOURCES_PATH + res);
    }

#ifdef USE_LIBROMFS
    return Box::inflateFromXMLString(romfs::get(res).string());
#else
    return Box::inflateFromXMLFile(std::string(BRLS_RESOURCES) + res);
#endif
}

void Box::inflateFromXMLFile(const std::string& path)
{
    // Load XML
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(path);
    tinyxml2::XMLElement* element                   = document->RootElement();

    if (!element)
    {
        tinyxml2::XMLError error = document->LoadFile(path.c_str());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Invalid XML when inflating " + this->describe() + ": error " + std::to_string(error));

        element = document->RootElement();

        if (!element)
            fatal("Invalid XML: no element found");
    }

    return Box::inflateFromXMLElement(element);
}

void Box::inflateFromXMLElement(tinyxml2::XMLElement* element)
{
    // Ensure element is a Box
    if (std::string(element->Name()) != "brls:Box")
        fatal("First XML element is " + std::string(element->Name()) + ", expected brls:Box");

    // Apply attributes
    this->applyXMLAttributes(element);

    // Handle children
    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
        this->addView(View::createFromXMLElement(child)); // don't call handleXMLElement because this method is for user XMLs
}

void Box::handleXMLElement(tinyxml2::XMLElement* element) { this->addView(View::createFromXMLElement(element)); }

void Box::setAxis(Axis axis)
{
    YGNodeStyleSetFlexDirection(this->ygNode, getYGFlexDirection(axis));
    this->m_axis = axis;
    this->invalidate();
}

Axis Box::getAxis() const { return m_axis; }

void Box::setWrap(Wrap wrap)
{
    YGNodeStyleSetFlexWrap(this->ygNode, getYGWrap(wrap));
    this->invalidate();
}

void Box::setDirection(Direction direction)
{
    switch (direction)
    {
        case Direction::INHERIT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionInherit);
            break;
        case Direction::LEFT_TO_RIGHT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionLTR);
            break;
        case Direction::RIGHT_TO_LEFT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionRTL);
            break;
    }

    this->invalidate();
}

void Box::setJustifyContent(JustifyContent justify)
{
    switch (justify)
    {
        case JustifyContent::FLEX_START:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyFlexStart);
            break;
        case JustifyContent::CENTER:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyCenter);
            break;
        case JustifyContent::FLEX_END:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyFlexEnd);
            break;
        case JustifyContent::SPACE_BETWEEN:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceBetween);
            break;
        case JustifyContent::SPACE_AROUND:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceAround);
            break;
        case JustifyContent::SPACE_EVENLY:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceEvenly);
            break;
    }

    this->invalidate();
}

void Box::setAlignItems(AlignItems alignment)
{
    switch (alignment)
    {
        case AlignItems::AUTO:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignAuto);
            break;
        case AlignItems::FLEX_START:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignFlexStart);
            break;
        case AlignItems::CENTER:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignCenter);
            break;
        case AlignItems::FLEX_END:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignFlexEnd);
            break;
        case AlignItems::STRETCH:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignStretch);
            break;
        case AlignItems::BASELINE:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignBaseline);
            break;
        case AlignItems::SPACE_BETWEEN:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignSpaceBetween);
            break;
        case AlignItems::SPACE_AROUND:
            YGNodeStyleSetAlignItems(this->ygNode, YGAlignSpaceAround);
            break;
    }

    this->invalidate();
}

View* Box::getView(const std::string& id)
{
    if (id == this->id)
        return this;

    for (View* child : this->m_children)
    {
        View* result = child->getView(id);

        if (result)
            return result;
    }

    return nullptr;
}

bool Box::applyXMLAttribute(const std::string& name, const std::string& value)
{
    if (this->m_forwardedAttributes.count(name) > 0)
    {
        std::pair<std::string, View*> pair = this->m_forwardedAttributes[name];
        return pair.second->applyXMLAttribute(pair.first, value);
    }

    return View::applyXMLAttribute(name, value);
}

void Box::forwardXMLAttribute(const std::string& attributeName, View* target)
{
    this->forwardXMLAttribute(attributeName, target, attributeName);
}

void Box::forwardXMLAttribute(const std::string& attributeName, View* target, const std::string& targetAttributeName)
{
    if (!target->isXMLAttributeValid(targetAttributeName))
        fatal(
            "Error when forwarding \"" + attributeName + "\" of \"" + this->describe() + "\": attribute \"" + targetAttributeName
            + "\" is not a XML valid attribute for view \"" + target->describe() + "\""
        );

    if (this->m_forwardedAttributes.count(attributeName) > 0)
        fatal(
            "Error when forwarding \"" + attributeName + "\" of \"" + this->describe() + "\": the same attribute cannot be forwarded twice"
        );

    this->m_forwardedAttributes[attributeName] = std::make_pair(targetAttributeName, target);
}

void Box::onChildFocusGained(View* directChild, View* focusedView)
{
    m_lastFocusedView = directChild;
    if (this->hasParent())
        this->getParent()->onChildFocusGained(this, focusedView);
}

void Box::onChildFocusLost(View* directChild, View* focusedView)
{
    if (this->hasParent())
        this->getParent()->onChildFocusLost(this, focusedView);
}

void Box::setLastFocusedView(View* view) { this->m_lastFocusedView = view; }

void Box::setDefaultFocusedIndex(int index)
{
    if (index < 0)
        return;
    this->m_defaultFocusedIndex = index;
}

size_t Box::getDefaultFocusedIndex() const { return this->m_defaultFocusedIndex; }

View* Box::getLastFocusedView() const { return this->m_lastFocusedView; }

bool Box::isChildFocused()
{
    for (auto& child : getChildren())
    {
        Box* box = dynamic_cast<Box*>(child);
        if (box)
        {
            if (box->isFocused())
                return true;
            if (box->isChildFocused())
                return true;
        }
        else if (child->isFocused())
        {
            return true;
        }
    }
    return false;
}

View* Box::create() { return new Box(); }

Padding::Padding() { this->setGrow(1.0f); }

void Padding::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) {}

View* Padding::create() { return new Padding(); }

Box::~Box()
{
    for (auto it : getChildren())
    {
        it->setParent(nullptr);
        if (!it->isPtrLocked())
        {
            delete it;
        }
        else
        {
            it->freeView();
        }
    }
}

} // namespace brls
