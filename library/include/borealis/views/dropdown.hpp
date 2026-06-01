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

#pragma once

#include <borealis/core/box.hpp>
#include <borealis/core/event.hpp>
#include <borealis/views/applet_frame.hpp>
#include <borealis/views/hint.hpp>
#include <borealis/views/recycler.hpp>
#include <string>

namespace brls
{

/**
 * Fired when the user has selected a value.
 *
 * Parameter is either the selected value index
 * or -1 if the user cancelled.
 *
 * Assume that the Dropdown is deleted
 * as soon as this function is called.
 */
typedef Event<int> ValueSelectedEvent;

/**
 * A modal dropdown list that allows the user to select between multiple values.
 */
class Dropdown : public Box, private RecyclerDataSource
{
  private:
    BRLS_BIND(RecyclerFrame, m_recycler, "brls/dropdown/recycler");
    BRLS_BIND(Box, m_header, "brls/dropdown/header");
    BRLS_BIND(Label, m_title, "brls/dropdown/title_label");
    BRLS_BIND(Box, m_content, "brls/dropdown/content");
    BRLS_BIND(AppletFrame, m_applet, "brls/dropdown/applet");

    ValueSelectedEvent::Callback m_cb;
    ValueSelectedEvent::Callback m_dismissCb;
    std::vector<std::string> m_values;
    int m_selected;

    Animatable m_showOffset = 0;
    Event<RecyclerCell*> m_cellFocusDidChangeEvent;

    int numberOfRows(RecyclerFrame* recycler, int section) override;
    RecyclerCell* cellForRow(RecyclerFrame* recycler, IndexPath index) override;
    void didSelectRowAt(RecyclerFrame* recycler, IndexPath index) override;

    void offsetTick();

  protected:
    float getShowAnimationDuration(TransitionAnimation animation) override;

  public:
    /**
     * Constructs a dropdown with the given title, list of values, and selection callback.
     * @param title The title displayed at the top of the dropdown.
     * @param values The list of string options to display.
     * @param cb Callback fired when the user selects a value (index) or cancels (-1).
     * @param selected The index of the initially selected value.
     * @param dismissCb Callback fired when the dropdown is dismissed.
     */
    Dropdown(
        const std::string& title,
        std::vector<std::string> values,
        ValueSelectedEvent::Callback cb,
        int selected                           = 0,
        ValueSelectedEvent::Callback dismissCb = [](int) {}
    );

    void show(const std::function<void(void)>& cb, bool animate, float animationDuration) override;
    void hide(const std::function<void(void)>& cb, bool animated, float animationDuration) override;

    virtual AppletFrame* getAppletFrame() override;
    virtual View* getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction) override;

    /** Returns the event fired when a cell gains focus in the dropdown list. */
    Event<RecyclerCell*>* getCellFocusDidChangeEvent() { return &m_cellFocusDidChangeEvent; }

    bool isTranslucent() override { return View::isTranslucent(); }
};

} // namespace brls
