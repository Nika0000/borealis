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

#include <borealis/core/application.hpp>
#include <borealis/core/bind.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/rectangle.hpp>

namespace brls
{

class Slider : public Box
{
  public:
    Slider();

    void onLayout() override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    View* getDefaultFocus() override;

    void setProgress(float progress);

    float getProgress() const;

    Event<float>* getProgressEvent();

    void setStep(float step);

    void setPointerSize(float size);

    static View* create();

  private:
    Rectangle* m_line;
    Rectangle* m_lineEmpty;
    Rectangle* m_pointer;

    Event<float> m_progressEvent;

    Animatable m_pointerScale = 1.0f;
    float m_basePointerSize   = 24.0f;

    float m_progress = 1;
    float m_step     = 0.0f;

    float m_holdTime = 0.0f;
    bool m_boundHit  = false;

    void updateUI();
    void continuousButtonProcessing();
    void setPointerPressed(bool pressed);
    void pointerScaleTick();
};

} // namespace brls
