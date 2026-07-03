/*
    Copyright 2019-2021 natinusala
    Copyright 2019 p-sam
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

#include <algorithm>
#include <borealis/core/animation.hpp>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/input.hpp>
#include <borealis/core/util.hpp>
#include <borealis/core/view.hpp>
#include <borealis/views/applet_frame.hpp>
#include <cmath>
#include <fstream>
#include <sstream>

// Avoid conflicts with macro definitions in windows.h
#undef RGB
#undef TRANSPARENT
#undef RELATIVE
#undef ABSOLUTE

using namespace brls::literals;

namespace brls
{

void AppletFrameItem::setHintView(View* hintView)
{
    this->m_hintView = hintView;
    this->m_hintView->ptrLock();
}

AppletFrameItem::~AppletFrameItem()
{
    if (m_hintView)
    {
        if (!m_hintView->isPtrLocked())
            delete m_hintView;
        else
            m_hintView->freeView();
    }
}

View::View()
{
    // Instantiate and prepare YGNode
    this->ygNode = YGNodeNewWithConfig(Application::getYogaConfig());
    YGNodeSetContext(this->ygNode, this);

    YGNodeStyleSetWidthAuto(this->ygNode);
    YGNodeStyleSetHeightAuto(this->ygNode);

    // Register common XML attributes
    this->registerCommonAttributes();

    // Default values
    Style style = Application::getStyle();

    this->m_highlightCornerRadius = style["brls/highlight/corner_radius"];
    this->m_scale                 = Point(1.0f, 1.0f);
}

static int shakeAnimation(float t, float a) // a = amplitude
{
    // Damped sine wave
    float w = 0.8f;  // period
    float c = 0.35f; // damp factor
    return roundf(a * exp(-(c * t)) * sin(w * t));
}

void View::shakeHighlight(FocusDirection direction)
{
    this->m_highlightShaking        = true;
    this->m_highlightShakeStart     = getCPUTimeUsec() / 1000;
    this->m_highlightShakeDirection = direction;
    this->m_highlightShakeAmplitude = std::rand() % 15 + 10;
}

float View::getAlpha(bool child) { return this->alpha * (this->parent ? this->parent->getAlpha(true) : 1.0f); }

NVGcolor View::a(NVGcolor color)
{
    NVGcolor newColor = color; // copy
    newColor.a *= this->getAlpha();
    return newColor;
}

NVGpaint View::a(NVGpaint paint)
{
    NVGpaint newPaint = paint; // copy
    newPaint.innerColor.a *= this->getAlpha();
    newPaint.outerColor.a *= this->getAlpha();
    return newPaint;
}

void View::interruptGestures(bool onlyIfUnsureState)
{
    for (GestureRecognizer* recognizer : getGestureRecognizers())
        recognizer->interrupt(onlyIfUnsureState);

    if (parent)
        parent->interruptGestures(onlyIfUnsureState);
}

void View::addGestureRecognizer(GestureRecognizer* recognizer) { this->m_gestureRecognizers.push_back(recognizer); }

Sound View::gestureRecognizerRequest(TouchState touch, MouseState mouse, View* firstResponder)
{
    Sound soundToPlay = touch.phase == TouchPhase::START ? SOUND_TOUCH : SOUND_NONE;

    for (GestureRecognizer* recognizer : getGestureRecognizers())
    {
        if (!recognizer->isEnabled())
            continue;

        GestureState state = recognizer->recognitionLoop(touch, mouse, this, &soundToPlay);
        if (state == GestureState::START || state == GestureState::END)
            firstResponder->interruptGestures(true);
    }

    Sound parentSound = SOUND_NONE;
    if (parent)
        parentSound = parent->gestureRecognizerRequest(touch, mouse, firstResponder);

    if (soundToPlay == SOUND_NONE)
        soundToPlay = parentSound;

    return soundToPlay;
}

void View::frame(FrameContext* ctx)
{
    if (this->m_visibility != Visibility::VISIBLE)
        return;

    Style style    = Application::getStyle();
    Theme oldTheme = ctx->theme;

    nvgSave(ctx->vg);

    // Theme override
    if (this->m_themeOverride)
        ctx->theme = *m_themeOverride;

    Rect frame   = getFrame();
    float x      = frame.getMinX();
    float y      = frame.getMinY();
    float width  = frame.getWidth();
    float height = frame.getHeight();

    if (this->m_scale.x != 1.0f || this->m_scale.y != 1.0f)
    {
        float cx = x + width / 2.0f;
        float cy = y + height / 2.0f;
        nvgTranslate(ctx->vg, cx, cy);
        nvgScale(ctx->vg, this->m_scale.x, this->m_scale.y);
        nvgTranslate(ctx->vg, -cx, -cy);
    }

    if (this->alpha > 0.0f && this->collapseState != 0.0f)
    {
        // Draw background
        this->drawBackground(ctx->vg, ctx, style, frame);

        // Draw shadow
        if (this->m_shadowType != ShadowType::NONE && (this->m_showShadow || Application::getInputType() == InputType::TOUCH))
            this->drawShadow(ctx->vg, ctx, style, frame);

        // Draw border
        if (this->m_borderThickness > 0.0f)
            this->drawBorder(ctx->vg, ctx, style, frame);

        this->drawLine(ctx, frame);

        // Draw highlight background
        if (this->m_highlightAlpha > 0.0f && !this->m_hideHighlightBackground && !this->m_hideHighlight)
            this->drawHighlight(ctx->vg, ctx->theme, this->m_highlightAlpha, style, true);

        // Collapse clipping
        if (this->collapseState < 1.0f || this->m_clipsToBounds)
        {
            nvgSave(ctx->vg);
            nvgIntersectScissor(ctx->vg, x, y, width, height * this->collapseState);
        }

        // Draw the view
        this->draw(ctx->vg, x, y, width, height, style, ctx);

        if (this->m_wireframeEnabled)
            this->drawWireframe(ctx, frame);

        // Reset clipping
        if (this->collapseState < 1.0f || this->m_clipsToBounds)
            nvgRestore(ctx->vg);

        // draw click animation
        if (this->m_clickAlpha > 0.0f && !this->m_hideClickAnimation)
            this->drawClickAnimation(ctx->vg, ctx, frame);

        // draw highlight
        if (this->m_highlightAlpha > 0.0f && !this->m_hideHighlightBorder && !this->m_hideHighlight
            && Application::getInputType() != InputType::TOUCH)
            this->drawHighlight(ctx->vg, ctx->theme, this->m_highlightAlpha, style, false);
    }

    // Cleanup
    if (this->m_themeOverride)
        ctx->theme = oldTheme;

    nvgRestore(ctx->vg);
}

void View::frameHighlight(FrameContext* ctx)
{
    // This method is now mostly handled in frame() for proper layering
    // Keep it for compatibility but the actual drawing happens in frame()
    if (this->alpha > 0.0f && this->collapseState != 0.0f && this->m_highlightAlpha > 0.0f && !this->m_hideHighlightBorder
        && !this->m_hideHighlight)
    {
        // Skip the top-level highlight pass for views inside a clipped ancestor —
        // they already get their highlight drawn (and clipped) during the normal
        // frame() pass.
        for (Box* ancestor = this->getParent(); ancestor != nullptr; ancestor = ancestor->getParent())
        {
            if (ancestor->getClipsToBounds())
                return;
        }

        nvgSave(ctx->vg);
        // Apply the same scaling pivot as in frame()
        if (this->m_scale.x != 1.0f || this->m_scale.y != 1.0f)
        {
            Rect frame = getFrame();
            float cx   = frame.getMinX() + frame.getWidth() / 2.0f;
            float cy   = frame.getMinY() + frame.getHeight() / 2.0f;
            nvgTranslate(ctx->vg, cx, cy);
            nvgScale(ctx->vg, this->m_scale.x, this->m_scale.y);
            nvgTranslate(ctx->vg, -cx, -cy);
        }
        this->drawHighlight(ctx->vg, ctx->theme, this->m_highlightAlpha, Application::getStyle(), false);
        nvgRestore(ctx->vg);
    }
}

void View::resetClickAnimation() { this->m_clickAlpha.stop(); }

void View::playClickAnimation(bool reverse, bool animateBack, bool force)
{
    if (m_hideClickAnimation && !force)
        return;

    this->resetClickAnimation();

    Style style = Application::getStyle();

    this->m_clickAlpha.reset(reverse ? 1.0f : 0.0f);

    this->m_clickAlpha.addStep(
        reverse ? 0.0f : 1.0f, style["brls/animations/highlight"], reverse ? EasingFunction::quadraticOut : EasingFunction::quadraticIn
    );

    this->m_clickAlpha.setEndCallback(
        [this, reverse, animateBack](bool finished)
        {
            if (reverse || !animateBack || Application::getInputType() == InputType::TOUCH)
                return;

            this->playClickAnimation(true);
        }
    );

    this->m_clickAlpha.start();
}

void View::drawClickAnimation(NVGcontext* vg, FrameContext* ctx, Rect frame)
{
    Theme theme    = ctx->theme;
    NVGcolor color = theme["brls/click_pulse"];

    color.a *= this->m_clickAlpha;

    nvgFillColor(vg, a(color));
    nvgBeginPath(vg);

    bool hasRadii
        = (this->m_cornerRadii[0] != 0 || this->m_cornerRadii[1] != 0 || this->m_cornerRadii[2] != 0 || this->m_cornerRadii[3] != 0);
    if (hasRadii)
        nvgRoundedRectVarying(
            vg,
            frame.getMinX(),
            frame.getMinY(),
            frame.getWidth(),
            frame.getHeight(),
            this->m_cornerRadii[0],
            this->m_cornerRadii[1],
            this->m_cornerRadii[2],
            this->m_cornerRadii[3]
        );
    else
        nvgRoundedRect(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), 4);

    nvgFill(vg);
}

void View::drawLine(FrameContext* ctx, Rect frame)
{
    // Don't setup and draw empty nvg path if there is no line to draw
    if (this->m_lineTop <= 0 && this->m_lineRight <= 0 && this->m_lineBottom <= 0 && this->m_lineLeft <= 0)
        return;

    nvgBeginPath(ctx->vg);
    nvgFillColor(ctx->vg, a(this->m_lineColor));

    if (this->m_lineTop > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.size.width, this->m_lineTop);

    if (this->m_lineRight > 0)
        nvgRect(ctx->vg, frame.getMaxX(), frame.getMinY(), this->m_lineRight, frame.size.height);

    if (this->m_lineBottom > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMaxY() - this->m_lineBottom, frame.size.width, this->m_lineBottom);

    if (this->m_lineLeft > 0)
        nvgRect(ctx->vg, frame.getMinX() - this->m_lineLeft, frame.getMinY(), this->m_lineLeft, frame.size.height);

    nvgFill(ctx->vg);
}

void View::drawWireframe(FrameContext* ctx, Rect frame)
{
    nvgStrokeWidth(ctx->vg, 1);

    // Outline
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 0, 255));
    nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight());
    nvgStroke(ctx->vg);

    if (this->hasParent())
    {
        // Diagonals
        nvgFillColor(ctx->vg, nvgRGB(0, 0, 255));

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, frame.getMinX(), frame.getMinY());
        nvgLineTo(ctx->vg, frame.getMaxX(), frame.getMaxY());
        nvgFill(ctx->vg);

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, frame.getMaxX(), frame.getMinY());
        nvgLineTo(ctx->vg, frame.getMinX(), frame.getMaxY());
        nvgFill(ctx->vg);
    }

    // Padding
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 255, 0));

    float paddingTop    = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeTop));
    float paddingLeft   = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeLeft));
    float paddingBottom = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeBottom));
    float paddingRight  = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeRight));

    // Top
    if (paddingTop > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), paddingTop);

    // Right
    if (paddingRight > 0)
        nvgRect(ctx->vg, frame.getMaxX() - paddingRight, frame.getMinY(), paddingRight, frame.getHeight());

    // Bottom
    if (paddingBottom > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMaxY() - paddingBottom, frame.getWidth(), paddingBottom);

    // Left
    if (paddingLeft > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), paddingLeft, frame.getHeight());

    nvgStroke(ctx->vg);

    // Margins
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(255, 0, 0));

    float marginTop    = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeTop));
    float marginLeft   = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeLeft));
    float marginBottom = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeBottom));
    float marginRight  = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeRight));

    // Top
    if (marginTop > 0)
        nvgRect(ctx->vg, frame.getMinX() - marginLeft, frame.getMinY() - marginTop, frame.getWidth() + marginLeft + marginRight, marginTop);

    // Right
    if (marginRight > 0)
        nvgRect(ctx->vg, frame.getMaxX(), frame.getMinY() - marginTop, marginRight, frame.getHeight() + marginTop + marginBottom);

    // Bottom
    if (marginBottom > 0)
        nvgRect(ctx->vg, frame.getMinX() - marginLeft, frame.getMaxY(), frame.getWidth() + marginLeft + marginRight, marginBottom);

    // Left
    if (marginLeft > 0)
        nvgRect(
            ctx->vg,
            frame.getMinX() - marginLeft,
            frame.getMinY() - marginTop,
            marginLeft,
            frame.getHeight() + marginTop + marginBottom //
        );

    nvgStroke(ctx->vg);
}

void View::drawBorder(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    nvgBeginPath(vg);
    nvgStrokeColor(vg, a(this->m_borderColor));
    nvgStrokeWidth(vg, this->m_borderThickness);

    bool hasRadii
        = (this->m_cornerRadii[0] != 0 || this->m_cornerRadii[1] != 0 || this->m_cornerRadii[2] != 0 || this->m_cornerRadii[3] != 0);

    if (hasRadii)
    {
        nvgRoundedRectVarying(
            vg,
            frame.getMinX(),
            frame.getMinY(),
            frame.getWidth(),
            frame.getHeight(),
            this->m_cornerRadii[0],
            this->m_cornerRadii[1],
            this->m_cornerRadii[2],
            this->m_cornerRadii[3]
        );
    }
    else
    {
        nvgRoundedRect(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->m_cornerRadius);
    }

    nvgStroke(vg);
}

void View::drawShadow(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    float shadowWidth   = 0.0f;
    float shadowFeather = 0.0f;
    float shadowOpacity = 0.0f;
    float shadowOffset  = 0.0f;

    switch (this->m_shadowType)
    {
        case ShadowType::GENERIC:
            shadowWidth   = style["brls/shadow/width"];
            shadowFeather = style["brls/shadow/feather"];
            shadowOpacity = style["brls/shadow/opacity"];
            shadowOffset  = style["brls/shadow/offset"];
            break;
        case ShadowType::CUSTOM:
            break;
        case ShadowType::NONE:
            break;
    }

    float maxRadius = std::max(
        {
            this->m_cornerRadii[0],
            this->m_cornerRadii[1],
            this->m_cornerRadii[2],
            this->m_cornerRadii[3],
        }
    );

    NVGpaint shadowPaint = nvgBoxGradient(
        vg,
        frame.getMinX(),
        frame.getMinY() + shadowWidth,
        frame.getWidth(),
        frame.getHeight(),
        maxRadius * 2,
        shadowFeather,
        RGBA(0, 0, 0, shadowOpacity * alpha),
        TRANSPARENT
    );

    nvgBeginPath(vg);
    nvgRect(
        vg,
        frame.getMinX() - shadowOffset,
        frame.getMinY() - shadowOffset,
        frame.getWidth() + shadowOffset * 2,
        frame.getHeight() + shadowOffset * 3
    );
    nvgRoundedRectVarying(
        vg,
        frame.getMinX(),
        frame.getMinY(),
        frame.getWidth(),
        frame.getHeight(),
        this->m_cornerRadii[0],
        this->m_cornerRadii[1],
        this->m_cornerRadii[2],
        this->m_cornerRadii[3]
    );
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadowPaint);
    nvgFill(vg);
}

void View::collapse(bool animated)
{
    if (animated)
    {
        Style style = Application::getStyle();

        this->collapseState.reset();
        this->collapseState.addStep(0.0f, style["brls/animations/collapse"], EasingFunction::quadraticOut);
        this->collapseState.setTickCallback(
            [this]
            {
                if (this->hasParent())
                    this->getParent()->invalidate();
            }
        );

        this->collapseState.start();
    }
    else
    {
        this->collapseState = 0.0f;
    }
}

bool View::isCollapsed() { return this->collapseState < 1.0f; }

void View::expand(bool animated)
{
    if (animated)
    {
        Style style = Application::getStyle();

        this->collapseState.reset();
        this->collapseState.addStep(1.0f, style["brls/animations/collapse"], EasingFunction::quadraticOut);
        this->collapseState.setTickCallback(
            [this]
            {
                if (this->hasParent())
                    this->getParent()->invalidate();
            }
        );

        this->collapseState.start();
    }
    else
    {
        this->collapseState = 1.0f;
    }
}

void View::setAlpha(float alpha) { this->alpha = alpha; }

void View::drawHighlight(NVGcontext* vg, Theme theme, float alpha, Style style, bool background)
{
    if (Application::getInputType() == InputType::TOUCH)
        return;

    nvgSave(vg);
    nvgResetScissor(vg);

    float padding      = this->m_highlightPadding;
    float cornerRadius = this->m_highlightCornerRadius;
    float strokeWidth  = style["brls/highlight/stroke_width"];

    for (Box* ancestor = this->getParent(); ancestor != nullptr; ancestor = ancestor->getParent())
    {
        if (ancestor->getClipsToBounds())
        {
            Rect clipFrame = ancestor->getFrame();
            nvgIntersectScissor(vg, clipFrame.getMinX(), clipFrame.getMinY(), clipFrame.getWidth(), clipFrame.getHeight());
        }
    }

    float x      = this->getX() - padding - strokeWidth / 2;
    float y      = this->getY() - padding - strokeWidth / 2;
    float width  = this->getWidth() + padding * 2 + strokeWidth;
    float height = this->getHeight() + padding * 2 + strokeWidth;

    // Shake animation
    if (this->m_highlightShaking)
    {
        Time curTime = getCPUTimeUsec() / 1000;
        Time t       = (curTime - m_highlightShakeStart) / 10;

        if (t >= style["brls/animations/highlight_shake"])
        {
            this->m_highlightShaking = false;
        }
        else
        {
            switch (this->m_highlightShakeDirection)
            {
                case FocusDirection::RIGHT:
                    x += shakeAnimation(t, this->m_highlightShakeAmplitude);
                    break;
                case FocusDirection::LEFT:
                    x -= shakeAnimation(t, this->m_highlightShakeAmplitude);
                    break;
                case FocusDirection::DOWN:
                    y += shakeAnimation(t, this->m_highlightShakeAmplitude);
                    break;
                case FocusDirection::UP:
                    y -= shakeAnimation(t, this->m_highlightShakeAmplitude);
                    break;
            }
        }
    }

    // Draw
    if (background)
    {
        // Background
        NVGcolor highlightBackgroundColor = theme["brls/highlight/background"];
        nvgFillColor(vg, RGBAf(highlightBackgroundColor.r, highlightBackgroundColor.g, highlightBackgroundColor.b, this->m_highlightAlpha));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgFill(vg);
    }
    else
    {
#ifdef SIMPLE_HIGHLIGHT
        // Border
        nvgBeginPath(vg);
        nvgStrokeColor(vg, a(theme["brls/highlight/color1"]));
        nvgStrokeWidth(vg, style["brls/highlight/stroke_width"]);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);
#else
        float shadowOffset = style["brls/highlight/shadow_offset"];

        // Shadow
        NVGpaint shadowPaint = nvgBoxGradient(
            vg, //
            x,  //
            y + style["brls/highlight/shadow_width"],
            width,
            height,                                                        //
            cornerRadius * 2,                                              //
            style["brls/highlight/shadow_feather"],                        //
            RGBA(0, 0, 0, style["brls/highlight/shadow_opacity"] * alpha), //
            TRANSPARENT                                                    //
        );

        nvgBeginPath(vg);
        nvgRect(vg, x - shadowOffset, y - shadowOffset, width + shadowOffset * 2, height + shadowOffset * 3);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);

        // Border
        float gradientX, gradientY, color;
        getHighlightAnimation(&gradientX, &gradientY, &color);

        NVGcolor highlightColor1 = theme["brls/highlight/color1"];

        NVGcolor pulsationColor = RGBAf(
            (color * highlightColor1.r) + (1 - color) * highlightColor1.r, //
            (color * highlightColor1.g) + (1 - color) * highlightColor1.g, //
            (color * highlightColor1.b) + (1 - color) * highlightColor1.b, //
            alpha                                                          //
        );

        NVGcolor borderColor = theme["brls/highlight/color2"];
        borderColor.a        = 0.5f * alpha * this->getAlpha();

        float strokeWidth = style["brls/highlight/stroke_width"];

        NVGpaint border1Paint = nvgRadialGradient(
            vg,
            x + gradientX * width,
            y + gradientY * height,
            strokeWidth * 10,
            strokeWidth * 40,
            borderColor,
            TRANSPARENT //
        );

        NVGpaint border2Paint = nvgRadialGradient(
            vg,
            x + (1 - gradientX) * width,
            y + (1 - gradientY) * height,
            strokeWidth * 10,
            strokeWidth * 40,
            borderColor,
            TRANSPARENT //
        );

        nvgBeginPath(vg);
        nvgStrokeColor(vg, pulsationColor);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border1Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border2Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);
#endif
    }

    nvgRestore(vg);
}

void View::setBackground(ViewBackground background) { this->m_background = background; }

void View::drawBackground(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    float x      = frame.getMinX();
    float y      = frame.getMinY();
    float width  = frame.getWidth();
    float height = frame.getHeight();

    Theme theme = ctx->theme;

    switch (this->m_background)
    {
        case ViewBackground::SIDEBAR:
        {
            float backdropHeight  = style["brls/sidebar/border_height"];
            NVGcolor sidebarColor = theme["brls/sidebar/background"];

            // Solid color
            nvgBeginPath(vg);
            nvgFillColor(vg, a(sidebarColor));
            nvgRect(vg, x, y + backdropHeight, width, height - backdropHeight * 2);
            nvgFill(vg);

            // Borders gradient
            //  Top
            NVGpaint topGradient = nvgLinearGradient(vg, x, y + backdropHeight, x, y, a(sidebarColor), TRANSPARENT);
            nvgBeginPath(vg);
            nvgFillPaint(vg, topGradient);
            nvgRect(vg, x, y, width, backdropHeight);
            nvgFill(vg);

            // Bottom
            NVGpaint bottomGradient = nvgLinearGradient(vg, x, y + height - backdropHeight, x, y + height, a(sidebarColor), TRANSPARENT);
            nvgBeginPath(vg);
            nvgFillPaint(vg, bottomGradient);
            nvgRect(vg, x, y + height - backdropHeight, width, backdropHeight);
            nvgFill(vg);
            break;
        }
        case ViewBackground::VERTICAL_LINEAR:
        {
            NVGpaint gradient = nvgLinearGradient(vg, x, y, x, y + height, a(m_backgroundStartColor), a(m_backgroundEndColor));
            nvgBeginPath(vg);
            nvgFillPaint(vg, gradient);
            if (std::all_of(this->m_backgroundRadius.begin(), this->m_backgroundRadius.end(), [](float i) { return i == 0.0f; }))
                nvgRect(vg, x, y, width, height);
            else
                nvgRoundedRectVarying(
                    vg,
                    x,
                    y,
                    width,
                    height,
                    m_backgroundRadius[0],
                    m_backgroundRadius[1],
                    m_backgroundRadius[2],
                    m_backgroundRadius[3] //
                );
            nvgFill(vg);
            break;
        }
        case ViewBackground::HORIZONTAL_LINEAR:
        {
            NVGpaint gradient = nvgLinearGradient(vg, x, y, x + width, y, a(m_backgroundStartColor), a(m_backgroundEndColor));
            nvgBeginPath(vg);
            nvgFillPaint(vg, gradient);
            if (std::all_of(this->m_backgroundRadius.begin(), this->m_backgroundRadius.end(), [](float i) { return i == 0.0f; }))
                nvgRect(vg, x, y, width, height);
            else
                nvgRoundedRectVarying(
                    vg,
                    x,
                    y,
                    width,
                    height,
                    m_backgroundRadius[0],
                    m_backgroundRadius[1],
                    m_backgroundRadius[2],
                    m_backgroundRadius[3] //
                );
            nvgFill(vg);
            break;
        }
        case ViewBackground::BACKDROP:
        {
            nvgFillColor(vg, a(theme["brls/backdrop"]));
            nvgBeginPath(vg);
            nvgRect(vg, x, y, width, height);
            nvgFill(vg);
            break;
        }
        case ViewBackground::SHAPE_COLOR:
        {
            nvgFillColor(vg, a(this->m_backgroundColor));
            nvgBeginPath(vg);

            bool hasRadii
                = (this->m_cornerRadii[0] != 0 || this->m_cornerRadii[1] != 0 || this->m_cornerRadii[2] != 0
                   || this->m_cornerRadii[3] != 0);

            if (hasRadii)
                nvgRoundedRectVarying(
                    vg,
                    x,
                    y,
                    width,
                    height,
                    this->m_cornerRadii[0],
                    this->m_cornerRadii[1],
                    this->m_cornerRadii[2],
                    this->m_cornerRadii[3] //
                );
            else
                nvgRect(vg, x, y, width, height);

            nvgFill(vg);
            break;
        }
        case ViewBackground::NONE:
            break;
    }
}

ActionIdentifier View::registerAction(
    const std::string& hintText,
    enum ControllerButton button,
    const ActionListener& actionListener,
    bool hidden,
    bool allowRepeating,
    enum Sound sound
)
{
    ActionIdentifier nextIdentifier = (this->m_actions.size() == 0) ? 1 : this->m_actions.back().identifier + 1;

    if (auto it = std::find(this->m_actions.begin(), this->m_actions.end(), button); it != this->m_actions.end())
        *it = { button, nextIdentifier, hintText, true, hidden, allowRepeating, sound, actionListener };
    else
        this->m_actions.push_back({ button, nextIdentifier, hintText, true, hidden, allowRepeating, sound, actionListener });

    return nextIdentifier;
}

void View::unregisterAction(ActionIdentifier identifier)
{
    auto is_matched_action = [identifier](const Action& action) { return action.identifier == identifier; };
    if (auto it = std::find_if(this->m_actions.begin(), this->m_actions.end(), is_matched_action); it != this->m_actions.end())
        this->m_actions.erase(it);
}

void View::registerClickAction(const ActionListener& actionListener)
{
    this->registerAction("hints/ok"_i18n, BUTTON_A, actionListener, false, false, SOUND_CLICK);
}

void View::updateActionHint(enum ControllerButton button, const std::string& hintText)
{
    if (auto it = std::find(this->m_actions.begin(), this->m_actions.end(), button); it != this->m_actions.end())
        it->hintText = hintText;
}

void View::setActionAvailable(enum ControllerButton button, bool available)
{
    if (auto it = std::find(this->m_actions.begin(), this->m_actions.end(), button); it != this->m_actions.end())
        it->available = available;

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setActionsAvailable(bool available)
{
    for (size_t i = 0; i < this->m_actions.size(); i++)
        this->m_actions[i].available = available;

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setParent(Box* parent, void* parentUserdata)
{
    if (this->m_parentUserdata)
        free(this->m_parentUserdata);

    this->parent           = parent;
    this->m_parentUserdata = parentUserdata;
}

void* View::getParentUserData() { return this->m_parentUserdata; }

bool View::isFocused() const { return this->focused; }

enum Sound View::getFocusSound() { return this->m_focusSound; }

Box* View::getParent() { return this->parent; }

bool View::hasParent() { return this->parent; }

void View::setDimensions(float width, float height)
{
    YGNodeStyleSetMinWidthPercent(this->ygNode, 0);
    YGNodeStyleSetMinHeightPercent(this->ygNode, 0);

    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setWidth(float width)
{
    YGNodeStyleSetMinWidthPercent(this->ygNode, 0);

    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    this->invalidate();
}

void View::setHeight(float height)
{
    YGNodeStyleSetMinHeightPercent(this->ygNode, 0);

    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setSize(Size size)
{
    setWidth(size.width);
    setHeight(size.height);
}

void View::setWidthPercentage(float percentage)
{
    YGNodeStyleSetWidthPercent(this->ygNode, percentage);
    YGNodeStyleSetMinWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setHeightPercentage(float percentage)
{
    YGNodeStyleSetHeightPercent(this->ygNode, percentage);
    YGNodeStyleSetMinHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMinWidth(float minWidth)
{
    if (minWidth == View::AUTO)
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMinWidth(this->ygNode, minWidth);

    this->invalidate();
}

void View::setMinWidthPercentage(float percentage)
{
    YGNodeStyleSetMinWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMinHeight(float minHeight)
{
    if (minHeight == View::AUTO)
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMinHeight(this->ygNode, minHeight);

    this->invalidate();
}

void View::setMinHeightPercentage(float percentage)
{
    YGNodeStyleSetMinHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMaxWidth(float maxWidth)
{
    if (maxWidth == View::AUTO)
        YGNodeStyleSetMaxWidth(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMaxWidth(this->ygNode, maxWidth);

    this->invalidate();
}

void View::setMaxWidthPercentage(float percentage)
{
    YGNodeStyleSetMaxWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMaxHeight(float maxHeight)
{
    if (maxHeight == View::AUTO)
        YGNodeStyleSetMaxHeight(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMaxHeight(this->ygNode, maxHeight);

    this->invalidate();
}

void View::setMaxHeightPercentage(float percentage)
{
    YGNodeStyleSetMaxHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMarginTop(float top)
{
    if (top == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    this->invalidate();
}

void View::setMarginRight(float right)
{
    if (right == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    this->invalidate();
}

float View::getMarginRight() { return ntz(YGNodeStyleGetMargin(this->ygNode, YGEdgeRight).value); }

float View::getMarginLeft() { return ntz(YGNodeStyleGetMargin(this->ygNode, YGEdgeLeft).value); }

void View::setMarginBottom(float bottom)
{
    if (bottom == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    this->invalidate();
}

void View::setMarginLeft(float left)
{
    if (left == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setMargins(float margins) { this->setMargins(margins, margins, margins, margins); }

void View::setMargins(float top, float right, float bottom, float left)
{
    if (top == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    if (right == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    if (bottom == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    if (left == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setPositionTop(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeTop, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeTop, pos);
    this->invalidate();
}

void View::setPositionRight(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeRight, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeRight, pos);
    this->invalidate();
}

void View::setPositionBottom(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeBottom, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeBottom, pos);
    this->invalidate();
}

void View::setPositionLeft(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeLeft, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeLeft, pos);
    this->invalidate();
}

void View::setPositionTopPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeTop, percentage);
    this->invalidate();
}

void View::setPositionRightPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeRight, percentage);
    this->invalidate();
}

void View::setPositionBottomPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeBottom, percentage);
    this->invalidate();
}

void View::setPositionLeftPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeLeft, percentage);
    this->invalidate();
}

void View::setPositionType(PositionType type)
{
    switch (type)
    {
        case PositionType::RELATIVE:
            YGNodeStyleSetPositionType(this->ygNode, YGPositionTypeRelative);
            break;
        case PositionType::ABSOLUTE:
            YGNodeStyleSetPositionType(this->ygNode, YGPositionTypeAbsolute);
            break;
    }

    this->invalidate();
}

void View::setGrow(float grow)
{
    YGNodeStyleSetFlexGrow(this->ygNode, grow);
    this->invalidate();
}

void View::setShrink(float shrink)
{
    YGNodeStyleSetFlexShrink(this->ygNode, shrink);
    this->invalidate();
}

void View::setAlignSelf(AlignSelf alignSelf)
{
    switch (alignSelf)
    {
        case AlignSelf::AUTO:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignAuto);
            break;
        case AlignSelf::FLEX_START:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignFlexStart);
            break;
        case AlignSelf::CENTER:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignCenter);
            break;
        case AlignSelf::FLEX_END:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignFlexEnd);
            break;
        case AlignSelf::STRETCH:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignStretch);
            break;
        case AlignSelf::BASELINE:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignBaseline);
            break;
        case AlignSelf::SPACE_BETWEEN:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignSpaceBetween);
            break;
        case AlignSelf::SPACE_AROUND:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignSpaceAround);
            break;
    }

    this->invalidate();
}

void View::invalidate()
{
    if (YGNodeHasMeasureFunc(this->ygNode))
        YGNodeMarkDirty(this->ygNode);

    if (this->hasParent() && !this->m_detached)
        this->getParent()->invalidate();
    else
        YGNodeCalculateLayout(this->ygNode, YGUndefined, YGUndefined, YGDirectionLTR);
}

Rect View::getFrame() { return Rect(getX(), getY(), getWidth(), getHeight()); }

float View::getX()
{
    if (this->hasParent())
        return this->getParent()->getX() + YGNodeLayoutGetLeft(this->ygNode) + this->m_translation.x
               + (isDetached() ? this->m_detachedOrigin.x : 0);
    return YGNodeLayoutGetLeft(this->ygNode) + this->m_translation.x;
}

float View::getY()
{
    if (this->hasParent())
        return this->getParent()->getY() + YGNodeLayoutGetTop(this->ygNode) + this->m_translation.y
               + (isDetached() ? this->m_detachedOrigin.y : 0);
    return YGNodeLayoutGetTop(this->ygNode) + this->m_translation.y;
}

Rect View::getLocalFrame() { return Rect(getLocalX(), getLocalY(), getWidth(), getHeight()); }

float View::getLocalX()
{
    return YGNodeLayoutGetLeft(this->ygNode) + this->m_translation.x + (isDetached() ? this->m_detachedOrigin.x : 0);
}

float View::getLocalY() { return YGNodeLayoutGetTop(this->ygNode) + this->m_translation.y + (isDetached() ? this->m_detachedOrigin.y : 0); }

float View::getHeight(bool includeCollapse)
{
    return YGNodeLayoutGetHeight(this->ygNode) * (includeCollapse ? this->collapseState.getValue() : 1.0f);
}

float View::getWidth() { return YGNodeLayoutGetWidth(this->ygNode); }

void View::detach() { this->m_detached = true; }

void View::setDetachedPosition(float x, float y)
{
    this->m_detachedOrigin.x = x;
    this->m_detachedOrigin.y = y;
}

void View::setDetachedPositionX(float x) { this->m_detachedOrigin.x = x; }

void View::setDetachedPositionY(float y) { this->m_detachedOrigin.y = y; }

bool View::isDetached() const { return this->m_detached; }

void View::onFocusGained()
{
    this->focused = true;

    Style style = Application::getStyle();

    this->m_highlightAlpha.reset();
    this->m_highlightAlpha.addStep(1.0f, style["brls/animations/highlight"], EasingFunction::quadraticOut);
    this->m_highlightAlpha.start();

    this->focusEvent.fire(this);

    if (this->hasParent())
        this->getParent()->onChildFocusGained(this, this);
}

GenericEvent* View::getFocusEvent() { return &this->focusEvent; }

GenericEvent* View::getFocusLostEvent() { return &this->focusLostEvent; }

void View::onFocusLost()
{
    this->focused = false;

    Style style = Application::getStyle();

    this->m_highlightAlpha.reset();
    this->m_highlightAlpha.addStep(0.0f, style["brls/animations/highlight"], EasingFunction::quadraticOut);
    this->m_highlightAlpha.start();

    this->focusLostEvent.fire(this);

    if (this->hasParent())
        this->getParent()->onChildFocusLost(this, this);
}

void View::setInFadeAnimation(bool translucent) { this->m_inFadeAnimation = translucent; }

void View::removeFromSuperView(bool free)
{
    if (parent)
        parent->removeView(this, free);
}

bool View::isTranslucent() { return this->m_fadeIn || this->m_inFadeAnimation; }

void View::show(const std::function<void(void)>& cb) { this->show(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE)); }

void View::show(const std::function<void(void)>& cb, bool animate, float animationDuration)
{
    if (!this->m_hidden)
    {
        this->onShowAnimationEnd();
        cb();
        return;
    }

    brls::Logger::debug("Showing {}", this->describe());

    this->m_hidden = false;

    this->m_fadeIn = true;

    if (animate)
    {
        this->alpha.reset(0.0f);

        this->alpha.addStep(1.0f, animationDuration, EasingFunction::quadraticOut);

        this->alpha.setEndCallback(
            [this, cb](bool finished)
            {
                this->m_fadeIn = false;
                this->onShowAnimationEnd();
                cb();
            }
        );

        this->alpha.start();
    }
    else
    {
        this->alpha    = 1.0f;
        this->m_fadeIn = false;
        this->onShowAnimationEnd();
        cb();
    }
}

void View::hide(const std::function<void(void)>& cb) { this->hide(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE)); }

void View::hide(
    const std::function<void(void)>& cb,
    bool animated,

    float animationDuration
)
{
    if (this->m_hidden)
    {
        cb();
        return;
    }

    brls::Logger::debug("Hiding {}", this->describe());

    this->m_hidden = true;
    this->m_fadeIn = false;

    if (animated)
    {
        this->alpha.reset(1.0f);

        this->alpha.addStep(0.0f, animationDuration, EasingFunction::quadraticOut);

        this->alpha.setEndCallback(
            [cb](bool finished)
            {
                if (finished)
                    cb();
            }
        );

        this->alpha.start();
    }
    else
    {
        this->alpha = 0.0f;
        cb();
    }
}

float View::getShowAnimationDuration(TransitionAnimation animation)
{
    Style style = Application::getStyle();

    if (animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT)
        fatal("Slide animation is not supported on views");

    return style["brls/animations/show"];
}

bool View::isHidden() const { return this->m_hidden; }

void View::overrideTheme(Theme* newTheme) { this->m_themeOverride = newTheme; }

void View::onParentFocusGained(View* focusedView) {}

void View::onParentFocusLost(View* focusedView) {}

View* View::getNextFocus(FocusDirection direction, View* currentView) { return getParent()->getNextFocus(direction, currentView); }

void View::setCustomNavigationRoute(FocusDirection direction, View* target)
{
    if (!this->m_focusable)
        fatal("Only focusable views can have a custom navigation route");

    this->m_customFocusByPtr[direction] = target;
}

void View::setCustomNavigationRoute(FocusDirection direction, const std::string& targetId)
{
    if (!this->m_focusable)
        fatal("Only focusable views can have a custom navigation route");

    this->m_customFocusById[direction] = targetId;
}

void View::setCustomNavigationRoute(const std::string& targetId)
{
    if (!this->m_focusable)
        fatal("Only focusable views can have a custom navigation route");

    m_customFocusById[FocusDirection::UP]    = targetId;
    m_customFocusById[FocusDirection::DOWN]  = targetId;
    m_customFocusById[FocusDirection::LEFT]  = targetId;
    m_customFocusById[FocusDirection::RIGHT] = targetId;
}

bool View::hasCustomNavigationRouteByPtr(FocusDirection direction) { return this->m_customFocusByPtr.count(direction) > 0; }

bool View::hasCustomNavigationRouteById(FocusDirection direction) { return this->m_customFocusById.count(direction) > 0; }

View* View::getCustomNavigationRoutePtr(FocusDirection direction) { return this->m_customFocusByPtr[direction]; }

std::string View::getCustomNavigationRouteId(FocusDirection direction) { return this->m_customFocusById[direction]; }

View::~View()
{
    this->resetClickAnimation();

    // Parent userdata
    if (this->m_parentUserdata)
    {
        free(this->m_parentUserdata);
        this->m_parentUserdata = nullptr;
    }

    // Focus sanity check
    if (Application::getCurrentFocus() == this)
        Application::giveFocus(nullptr);

    Application::tryDeinitFirstResponder(this);
    for (GestureRecognizer* recognizer : this->m_gestureRecognizers)
        delete recognizer;

    alpha.stop();
    m_clickAlpha.stop();
    m_highlightAlpha.stop();
    collapseState.stop();

    YGNodeFree(this->ygNode);

    if (deletionToken)
        *deletionToken = true;
}

std::string View::getStringXMLAttributeValue(std::string value)
{
    if (startsWith(value, "@i18n/"))
    {
        std::string stringName = value.substr(6);
        return getStr(stringName);
    }

    return value;
}

std::string View::getFilePathXMLAttributeValue(std::string value)
{
    if (startsWith(value, "@res/"))
    {
        std::string resPath = value.substr(5);
#ifdef USE_LIBROMFS
        return resPath;
#else
        return std::string(BRLS_RESOURCES) + resPath;
#endif
    }

    return value;
}

bool View::applyXMLAttribute(const std::string& name, const std::string& value)
{
    // String -> string
    if (this->m_stringAttributes.count(name) > 0)
    {
        if (startsWith(value, "@i18n/"))
        {
            this->m_stringAttributes[name](View::getStringXMLAttributeValue(value));
            return true;
        }

        this->m_stringAttributes[name](value);
        return true;
    }

    // File path -> file path
    if (startsWith(value, "@res/"))
    {
        if (this->m_filePathAttributes.count(name) > 0)
        {
#ifdef USE_LIBROMFS
            this->m_filePathAttributes[name](value);
#else
            this->m_filePathAttributes[name](View::getFilePathXMLAttributeValue(value));
#endif
            return true;
        }
        else
        {
            return false; // unknown res
        }
    }
    else
    {
        if (this->m_filePathAttributes.count(name) > 0)
        {
            this->m_filePathAttributes[name](value);
            return true;
        }

        // don't return false as it can be anything else
    }

    // Auto -> auto
    if (value == "auto")
    {
        if (this->m_autoAttributes.count(name) > 0)
        {
            this->m_autoAttributes[name]();
            return true;
        }
        else
        {
            return false;
        }
    }

    // Ends with px -> float
    if (endsWith(value, "px"))
    {
        // Strip the px and parse the float value
        std::string newFloat = value.substr(0, value.length() - 2);
        try
        {
            float floatValue = std::stof(newFloat);
            if (this->m_floatAttributes.count(name) > 0)
            {
                this->m_floatAttributes[name](floatValue);
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }

    // Ends with % -> percentage
    if (endsWith(value, "%"))
    {
        // Strip the % and parse the float value
        std::string newFloat = value.substr(0, value.length() - 1);
        try
        {
            float floatValue = std::stof(newFloat);

            if (floatValue < -100 || floatValue > 100)
                return false;

            if (this->m_percentageAttributes.count(name) > 0)
            {
                this->m_percentageAttributes[name](floatValue);
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }
    // Starts with @style -> float
    else if (startsWith(value, "@style/"))
    {
        // Parse the style name
        std::string styleName = value.substr(7);                    // length of "@style/"
        float value           = Application::getStyle()[styleName]; // will throw logic_error if the
                                                                    // metric doesn't exist

        if (this->m_floatAttributes.count(name) > 0)
        {
            this->m_floatAttributes[name](value);
            return true;
        }
        else
        {
            return false;
        }
    }
    // Starts with with # -> color
    else if (startsWith(value, "#"))
    {
        // Parse the color
        // #RRGGBB format
        if (value.size() == 7)
        {
            unsigned int r = 0, g = 0, b = 0;
            std::stringstream sr { value.substr(1, 2) };
            sr >> std::hex >> r;
            std::stringstream sg { value.substr(3, 2) };
            sg >> std::hex >> g;
            std::stringstream sb { value.substr(5, 2) };
            sb >> std::hex >> b;

            if (sr.fail() || sg.fail() || sb.fail())
            {
                return false;
            }
            else if (this->m_colorAttributes.count(name) > 0)
            {
                this->m_colorAttributes[name](nvgRGB(r, g, b));
                return true;
            }
            else
            {
                return false;
            }
        }
        // #RRGGBBAA format
        else if (value.size() == 9)
        {
            unsigned int r = 0, g = 0, b = 0, a = 0;
            std::stringstream sr { value.substr(1, 2) };
            sr >> std::hex >> r;
            std::stringstream sg { value.substr(3, 2) };
            sg >> std::hex >> g;
            std::stringstream sb { value.substr(5, 2) };
            sb >> std::hex >> b;
            std::stringstream sa { value.substr(7, 2) };
            sa >> std::hex >> a;

            if (sr.fail() || sg.fail() || sb.fail() || sa.fail())
            {
                return false;
            }
            else if (this->m_colorAttributes.count(name) > 0)
            {
                this->m_colorAttributes[name](nvgRGBA(r, g, b, a));
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    // Starts with @theme -> color
    else if (startsWith(value, "@theme/"))
    {
        // Parse the color name
        std::string colorName = value.substr(7);                    // length of "@theme/"
        NVGcolor value        = Application::getTheme()[colorName]; // will throw logic_error if the
                                                                    // color doesn't exist

        if (this->m_colorAttributes.count(name) > 0)
        {
            this->m_colorAttributes[name](value);
            return true;
        }
        else
        {
            return false;
        }
    }
    // Equals true or false -> bool
    else if (value == "true" || value == "false")
    {
        bool boolValue = value == "true";

        if (this->m_boolAttributes.count(name) > 0)
        {
            this->m_boolAttributes[name](boolValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    // Valid float -> float, otherwise unknown attribute
    try
    {
        float newValue = std::stof(value);
        if (this->m_floatAttributes.count(name) > 0)
        {
            this->m_floatAttributes[name](newValue);
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::invalid_argument& exception)
    {
        return false;
    }
}

void View::applyXMLAttributes(tinyxml2::XMLElement* element)
{
    if (!element)
        return;

    for (const tinyxml2::XMLAttribute* attribute = element->FirstAttribute(); attribute != nullptr; attribute = attribute->Next())
    {
        std::string name  = attribute->Name();
        std::string value = std::string(attribute->Value());

        // Ignore XML schema namespace attributes used for validation
        if (name == "xmlns" || name.starts_with("xmlns:") || name.starts_with("xsi:"))
            continue;

        if (!this->applyXMLAttribute(name, value))
            this->printXMLAttributeErrorMessage(element, name, value);
    }
}

bool View::isXMLAttributeValid(const std::string& attributeName) { return this->m_knownAttributes.count(attributeName) > 0; }

View* View::createFromXMLResource(const std::string& name)
{
    // Check if custom xml file exists
    if (!View::CUSTOM_RESOURCES_PATH.empty() && std::ifstream { View::CUSTOM_RESOURCES_PATH + "xml/" + name }.good())
    {
        return View::createFromXMLFile(View::CUSTOM_RESOURCES_PATH + "xml/" + name);
    }

#ifdef USE_LIBROMFS
    return View::createFromXMLString(romfs::get("xml/" + name).string());
#else
    return View::createFromXMLFile(std::string(BRLS_RESOURCES) + "xml/" + name);
#endif
}

View* View::createFromXMLString(std::string_view xml)
{
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(xml);

    tinyxml2::XMLElement* element = document->RootElement();

    if (!element)
    {
        tinyxml2::XMLError error = document->Parse(xml.data());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Invalid XML when creating View from XML: error " + std::to_string(error));

        element = document->RootElement();

        if (!element)
            fatal("Invalid XML: no element found");
    }

    View* view = View::createFromXMLElement(element);
    return view;
}

View* View::createFromXMLFile(const std::string& path)
{
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(path);

    tinyxml2::XMLElement* element = document->RootElement();

    if (!element)
    {
        tinyxml2::XMLError error = document->LoadFile(path.c_str());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Unable to load XML file \"" + path + "\": error " + std::to_string(error));

        element = document->RootElement();

        if (!element)
            fatal("Unable to load XML file \"" + path + "\": no root element found, is the file empty?");
    }

    View* view = View::createFromXMLElement(element);
    return view;
}

View* View::createFromXMLElement(tinyxml2::XMLElement* element)
{
    if (!element)
        return nullptr;

    std::string viewName = element->Name();

    // Instantiate the view
    View* view = nullptr;

    // Special case where element name is brls:View: create from given XML file.
    // XML attributes are explicitely not passed down to the created view.
    // To create a custom view from XML that you can reuse in other XML files,
    // make a class inheriting brls::Box and use the inflateFromXML* methods.
    if (viewName == "brls:View")
    {
        const tinyxml2::XMLAttribute* xmlAttribute = element->FindAttribute("xml");

        if (xmlAttribute)
        {
#ifdef USE_LIBROMFS
            view = View::createFromXMLString(romfs::get(View::getFilePathXMLAttributeValue(xmlAttribute->Value())).string());
#else
            view = View::createFromXMLFile(View::getFilePathXMLAttributeValue(xmlAttribute->Value()));
#endif
        }
        else
        {
            fatal("brls:View XML tag must have an \"xml\" attribute");
        }
    }
    // Otherwise look in the register
    else
    {
        if (!Application::XMLViewsRegisterContains(viewName))
            fatal("Unknown XML tag \"" + viewName + "\"");

        view = Application::getXMLViewCreator(viewName)();

        view->applyXMLAttributes(element);
    }

    unsigned count = 0;
    unsigned max   = view->getMaximumAllowedXMLElements();
    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        if (count >= max)
            fatal("View \"" + view->describe() + "\" is only allowed to have " + std::to_string(max) + " children XML elements");
        else
            view->handleXMLElement(child);

        count++;
    }

    return view;
}

void View::handleXMLElement(tinyxml2::XMLElement* element) { fatal("Raw views cannot have child XML tags"); }

void View::setMaximumAllowedXMLElements(unsigned max) { this->m_maximumAllowedXMLElements = max; }

unsigned View::getMaximumAllowedXMLElements() const { return this->m_maximumAllowedXMLElements; }

void View::registerCommonAttributes()
{
    // Width
    this->registerAutoXMLAttribute("width", [this] { this->setWidth(View::AUTO); });
    this->registerFloatXMLAttribute("width", [this](float value) { this->setWidth(value); });
    this->registerPercentageXMLAttribute("width", [this](float value) { this->setWidthPercentage(value); });

    // Height
    this->registerAutoXMLAttribute("height", [this] { this->setHeight(View::AUTO); });
    this->registerFloatXMLAttribute("height", [this](float value) { this->setHeight(value); });
    this->registerPercentageXMLAttribute("height", [this](float value) { this->setHeightPercentage(value); });

    // Min width
    this->registerAutoXMLAttribute("minWidth", [this] { this->setMinWidth(View::AUTO); });
    this->registerFloatXMLAttribute("minWidth", [this](float value) { this->setMinWidth(value); });
    this->registerPercentageXMLAttribute("minWidth", [this](float percentage) { this->setMinWidthPercentage(percentage); });

    // Min height
    this->registerAutoXMLAttribute("minHeight", [this] { this->setMinHeight(View::AUTO); });
    this->registerFloatXMLAttribute("minHeight", [this](float value) { this->setMinHeight(value); });
    this->registerPercentageXMLAttribute("minHeight", [this](float percentage) { this->setMinHeightPercentage(percentage); });

    // Max width
    this->registerAutoXMLAttribute("maxWidth", [this] { this->setMaxWidth(View::AUTO); });
    this->registerFloatXMLAttribute("maxWidth", [this](float value) { this->setMaxWidth(value); });
    this->registerPercentageXMLAttribute("maxWidth", [this](float percentage) { this->setMaxWidthPercentage(percentage); });

    // Max height
    this->registerAutoXMLAttribute("maxHeight", [this] { this->setMaxHeight(View::AUTO); });
    this->registerFloatXMLAttribute("maxHeight", [this](float value) { this->setMaxHeight(value); });
    this->registerPercentageXMLAttribute("maxHeight", [this](float percentage) { this->setMaxHeightPercentage(percentage); });

    // Grow and shrink
    this->registerFloatXMLAttribute("grow", [this](float value) { this->setGrow(value); });
    this->registerFloatXMLAttribute("shrink", [this](float value) { this->setShrink(value); });

    // Alignment
    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "alignSelf",
        AlignSelf,
        this->setAlignSelf,
        {
            { "auto", AlignSelf::AUTO },
            { "flexStart", AlignSelf::FLEX_START },
            { "center", AlignSelf::CENTER },
            { "flexEnd", AlignSelf::FLEX_END },
            { "stretch", AlignSelf::STRETCH },
            { "baseline", AlignSelf::BASELINE },
            { "spaceBetween", AlignSelf::SPACE_BETWEEN },
            { "spaceAround", AlignSelf::SPACE_AROUND },
        }
    );

    // Margins all
    this->registerFloatXMLAttribute("margin", [this](float value) { this->setMargins(value, value, value, value); });
    this->registerAutoXMLAttribute("margin", [this] { this->setMargins(View::AUTO, View::AUTO, View::AUTO, View::AUTO); });

    // Margin top
    this->registerFloatXMLAttribute("marginTop", [this](float value) { this->setMarginTop(value); });
    this->registerAutoXMLAttribute("marginTop", [this] { this->setMarginTop(View::AUTO); });

    // Margin right
    this->registerFloatXMLAttribute("marginRight", [this](float value) { this->setMarginRight(value); });
    this->registerAutoXMLAttribute("marginRight", [this] { this->setMarginRight(View::AUTO); });

    // Margin bottom
    this->registerFloatXMLAttribute("marginBottom", [this](float value) { this->setMarginBottom(value); });
    this->registerAutoXMLAttribute("marginBottom", [this] { this->setMarginBottom(View::AUTO); });

    // Margin left
    this->registerFloatXMLAttribute("marginLeft", [this](float value) { this->setMarginLeft(value); });
    this->registerAutoXMLAttribute("marginLeft", [this] { this->setMarginLeft(View::AUTO); });

    // Line
    this->registerColorXMLAttribute("lineColor", [this](NVGcolor color) { this->setLineColor(color); });
    this->registerFloatXMLAttribute("lineTop", [this](float value) { this->setLineTop(value); });
    this->registerFloatXMLAttribute("lineRight", [this](float value) { this->setLineRight(value); });
    this->registerFloatXMLAttribute("lineBottom", [this](float value) { this->setLineBottom(value); });
    this->registerFloatXMLAttribute("lineLeft", [this](float value) { this->setLineLeft(value); });

    // Position
    this->registerFloatXMLAttribute("positionTop", [this](float value) { this->setPositionTop(value); });
    this->registerFloatXMLAttribute("positionRight", [this](float value) { this->setPositionRight(value); });
    this->registerFloatXMLAttribute("positionBottom", [this](float value) { this->setPositionBottom(value); });
    this->registerFloatXMLAttribute("positionLeft", [this](float value) { this->setPositionLeft(value); });
    this->registerPercentageXMLAttribute("positionTop", [this](float value) { this->setPositionTopPercentage(value); });
    this->registerPercentageXMLAttribute("positionRight", [this](float value) { this->setPositionRightPercentage(value); });
    this->registerPercentageXMLAttribute("positionBottom", [this](float value) { this->setPositionBottomPercentage(value); });
    this->registerPercentageXMLAttribute("positionLeft", [this](float value) { this->setPositionLeftPercentage(value); });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "positionType",
        PositionType,
        this->setPositionType,
        {
            { "relative", PositionType::RELATIVE },
            { "absolute", PositionType::ABSOLUTE },
        }
    );

    // Custom focus routes
    this->registerStringXMLAttribute(
        "focusUp", [this](const std::string& value) { this->setCustomNavigationRoute(FocusDirection::UP, value); }
    );

    this->registerStringXMLAttribute(
        "focusRight", [this](const std::string& value) { this->setCustomNavigationRoute(FocusDirection::RIGHT, value); }
    );

    this->registerStringXMLAttribute(
        "focusDown", [this](const std::string& value) { this->setCustomNavigationRoute(FocusDirection::DOWN, value); }
    );

    this->registerStringXMLAttribute(
        "focusLeft", [this](const std::string& value) { this->setCustomNavigationRoute(FocusDirection::LEFT, value); }
    );

    this->registerStringXMLAttribute("focusTarget", [this](const std::string& value) { this->setCustomNavigationRoute(value); });

    // Shape
    this->registerColorXMLAttribute("backgroundColor", [this](NVGcolor value) { this->setBackgroundColor(value); });
    this->registerColorXMLAttribute("borderColor", [this](NVGcolor value) { this->setBorderColor(value); });
    this->registerFloatXMLAttribute("borderThickness", [this](float value) { this->setBorderThickness(value); });
    this->registerFloatXMLAttribute("cornerRadius", [this](float value) { this->setCornerRadius(value); });
    this->registerFloatXMLAttribute("cornerTopLeftRadius", [this](float value) { this->m_cornerRadii[0] = value; });
    this->registerFloatXMLAttribute("cornerTopRightRadius", [this](float value) { this->m_cornerRadii[1] = value; });
    this->registerFloatXMLAttribute("cornerBottomRightRadius", [this](float value) { this->m_cornerRadii[2] = value; });
    this->registerFloatXMLAttribute("cornerBottomLeftRadius", [this](float value) { this->m_cornerRadii[3] = value; });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "shadowType",
        ShadowType,
        this->setShadowType,
        {
            {
                "none",
                ShadowType::NONE,
            },
            {
                "generic",
                ShadowType::GENERIC,
            },
            {
                "custom",
                ShadowType::CUSTOM,
            },
        }
    );

    // Misc
    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "visibility",
        Visibility,
        this->setVisibility,
        {
            { "visible", Visibility::VISIBLE },
            { "invisible", Visibility::INVISIBLE },
            { "gone", Visibility::GONE },
        }
    );

    this->registerStringXMLAttribute("id", [this](const std::string& value) { this->setId(std::move(value)); });

    BRLS_REGISTER_ENUM_XML_ATTRIBUTE(
        "background",
        ViewBackground,
        this->setBackground,
        {
            { "sidebar", ViewBackground::SIDEBAR },
            { "backdrop", ViewBackground::BACKDROP },
            { "vertical_linear", ViewBackground::VERTICAL_LINEAR },
            { "horizontal_linear", ViewBackground::HORIZONTAL_LINEAR },
        }
    );

    // background start and end color for vertical linear style
    this->registerColorXMLAttribute("backgroundStartColor", [this](NVGcolor value) { this->m_backgroundStartColor = value; });
    this->registerColorXMLAttribute("backgroundEndColor", [this](NVGcolor value) { this->m_backgroundEndColor = value; });

    // background corner radius for vertical linear style
    this->registerFloatXMLAttribute("backgroundTopLeftRadius", [this](float value) { this->m_backgroundRadius[0] = value; });
    this->registerFloatXMLAttribute("backgroundTopRightRadius", [this](float value) { this->m_backgroundRadius[1] = value; });
    this->registerFloatXMLAttribute("backgroundBottomRightRadius", [this](float value) { this->m_backgroundRadius[2] = value; });
    this->registerFloatXMLAttribute("backgroundBottomLeftRadius", [this](float value) { this->m_backgroundRadius[3] = value; });

    this->registerBoolXMLAttribute("focusable", [this](bool value) { this->setFocusable(value); });

    this->registerBoolXMLAttribute("wireframe", [this](bool value) { this->setWireframeEnabled(value); });

    // Highlight
    this->registerBoolXMLAttribute("hideHighlightBackground", [this](bool value) { this->setHideHighlightBackground(value); });
    this->registerBoolXMLAttribute("hideHighlightBorder", [this](bool value) { this->setHideHighlightBorder(value); });
    this->registerBoolXMLAttribute("hideClickAnimation", [this](bool value) { this->setHideClickAnimation(value); });
    this->registerBoolXMLAttribute("hideHighlight", [this](bool value) { this->setHideHighlight(value); });
    this->registerFloatXMLAttribute("highlightPadding", [this](float value) { this->setHighlightPadding(value); });
    this->registerFloatXMLAttribute("highlightCornerRadius", [this](float value) { this->setHighlightCornerRadius(value); });

    // Misc
    this->registerStringXMLAttribute("title", [this](std::string value) { this->getAppletFrameItem()->title = std::move(value); });

    this->registerFilePathXMLAttribute(
        "icon", [this](std::string value) { this->getAppletFrameItem()->setIconFromFile(std::move(value)); }
    );

    this->registerFloatXMLAttribute(
        "detachedX",
        [this](float value)
        {
            this->detach();
            this->setDetachedPositionX(value);
        }
    );

    this->registerFloatXMLAttribute(
        "detachedY",
        [this](float value)
        {
            this->detach();
            this->setDetachedPositionY(value);
        }
    );

    this->registerFloatXMLAttribute("alpha", [this](float value) { this->setAlpha(value); });

    this->registerBoolXMLAttribute("clipsToBounds", [this](bool value) { this->setClipsToBounds(value); });

    this->registerBoolXMLAttribute("culled", [this](bool value) { this->setCulled(value); });

    this->registerFloatXMLAttribute("aspectRatio", [this](float value) { this->setAspectRatio(value); });
}

void View::setTranslationY(float translationY) { this->m_translation.y = translationY; }

void View::setTranslationX(float translationX) { this->m_translation.x = translationX; }

Point View::getScale() { return this->m_scale; }

void View::setScaleX(float scaleX) { this->m_scale.x = scaleX; }

void View::setScaleY(float scaleY) { this->m_scale.y = scaleY; }

void View::setScale(float scale)
{
    this->m_scale.x = scale;
    this->m_scale.y = scale;
}

void View::setVisibility(Visibility visibility)
{
    // Only change YG properties and invalidate if going from or to GONE
    if ((this->m_visibility == Visibility::GONE && visibility != Visibility::GONE)
        || (this->m_visibility != Visibility::GONE && visibility == Visibility::GONE))
    {
        if (visibility == Visibility::GONE)
            YGNodeStyleSetDisplay(this->ygNode, YGDisplayNone);
        else
            YGNodeStyleSetDisplay(this->ygNode, YGDisplayFlex);

        this->invalidate();
    }

    this->m_visibility = visibility;

    if (visibility == Visibility::VISIBLE)
        this->willAppear();
    else
        this->willDisappear();
}

Visibility View::getVisibility() { return m_visibility; }

void View::printXMLAttributeErrorMessage(tinyxml2::XMLElement* element, const std::string& name, const std::string& value)
{
    if (this->m_knownAttributes.find(name) != this->m_knownAttributes.end())
        fatal("Illegal value \"" + value + "\" for \"" + std::string(element->Name()) + "\" XML attribute \"" + name + "\"");
    else
        fatal("Unknown XML attribute \"" + name + "\" for tag \"" + std::string(element->Name()) + "\" (with value \"" + value + "\")");
}

void View::registerFloatXMLAttribute(const std::string& name, const FloatAttributeHandler& handler)
{
    this->m_floatAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerPercentageXMLAttribute(const std::string& name, const FloatAttributeHandler& handler)
{
    this->m_percentageAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerAutoXMLAttribute(const std::string& name, const AutoAttributeHandler& handler)
{
    this->m_autoAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerStringXMLAttribute(const std::string& name, const StringAttributeHandler& handler)
{
    this->m_stringAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerColorXMLAttribute(const std::string& name, const ColorAttributeHandler& handler)
{
    this->m_colorAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerBoolXMLAttribute(const std::string& name, const BoolAttributeHandler& handler)
{
    this->m_boolAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

void View::registerFilePathXMLAttribute(const std::string& name, const FilePathAttributeHandler& handler)
{
    this->m_filePathAttributes[name] = handler;
    this->m_knownAttributes.insert(name);
}

float ntz(float value) { return std::isnan(value) ? 0.0f : value; }

View* View::getView(const std::string& id)
{
    if (id == this->id)
        return this;

    return nullptr;
}

View* View::getNearestView(const std::string& id)
{
    // First try a children of ours
    View* child = this->getView(id);

    if (child)
        return child;

    // Then go up one level and try again
    if (this->hasParent())
        return this->getParent()->getNearestView(id);

    return nullptr;
}

void View::setId(const std::string& id)
{
    if (id == "")
        fatal("ID cannot be empty");

    this->id = id;
}

bool View::isFocusable() { return this->m_focusable && this->m_visibility == Visibility::VISIBLE; }

View* View::getDefaultFocus()
{
    if (this->isFocusable())
        return this;

    return nullptr;
}

View* View::hitTest(Point point)
{
    // Check if can focus ourself first
    if (!this->isFocusable() || alpha == 0.0f || m_visibility != Visibility::VISIBLE)
        return nullptr;

    Rect frame = getFrame();
    if (frame.pointInside(point))
    {
        return this;
    }

    return nullptr;
}

std::shared_ptr<tinyxml2::XMLDocument> View::getXMLCache(std::string_view path)
{
    static std::unordered_map<size_t, std::shared_ptr<tinyxml2::XMLDocument>> xmlCache;
    const size_t hash = std::hash<std::string> {}(path.data());
    if (!xmlCache.contains(hash))
    {
        xmlCache[hash] = std::make_shared<tinyxml2::XMLDocument>();
    }
    return xmlCache[hash];
}

void View::setWireframeEnabled(bool wireframe) { this->m_wireframeEnabled = wireframe; }

bool View::isWireframeEnabled() const { return this->m_wireframeEnabled; }

AppletFrame* View::getAppletFrame()
{
    View* view = this;
    while (view)
    {
        auto* applet = dynamic_cast<AppletFrame*>(view);
        if (applet)
            return applet;

        view = view->getParent();
    }
    return nullptr;
}

void View::updateAppletFrameItem()
{
    AppletFrame* appletFrame = this->getAppletFrame();
    if (appletFrame)
    {
        appletFrame->updateAppletFrameItem();
        appletFrame->setTitle(getAppletFrameItem()->title);
        appletFrame->setIcon(getAppletFrameItem()->iconPath);
    }
}

void View::present(View* view)
{
    AppletFrame* applet = getAppletFrame();
    if (!applet)
        return;

    applet->pushContentView(view);
}

void View::dismiss(std::function<void(void)> cb)
{
    AppletFrame* applet = getAppletFrame();
    if (!applet)
        return;

    applet->popContentView(std::move(cb));
}

void View::ptrLock() { m_ptrLockCounter++; }

void View::ptrUnlock() { m_ptrLockCounter--; }

bool View::isPtrLocked() const { return m_ptrLockCounter > 0; }

void View::freeView() { Application::addToFreeQueue(this); }

Activity* View::getParentActivity()
{
    if (parentActivity != nullptr)
        return parentActivity;

    if (parent != nullptr)
        return parent->getParentActivity();

    return nullptr;
}

void View::setParentActivity(Activity* activity) { parentActivity = activity; }

} // namespace brls
