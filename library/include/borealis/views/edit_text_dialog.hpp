#pragma once

#include <borealis/core/bind.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/label.hpp>

namespace brls
{
class EditTextDialog : public Box
{
  public:
    EditTextDialog();
    ~EditTextDialog() override;

    /**
     * Pushes the dialog as a new activity onto the navigation stack.
     */
    void open();

    /**
     * Sets the text content displayed in the input field.
     */
    void setText(const std::string& value);

    /**
     * Sets the header label above the input field; hidden when empty.
     */
    void setHeaderText(const std::string& value);

    /**
     * Sets the placeholder text shown when the input field is empty.
     */
    void setHintText(const std::string& value);

    /**
     * Sets the character count label text (e.g. "10/100").
     */
    void setCountText(const std::string& value);

    /**
     * Sets the cursor position in the input label.
     */
    void setCursor(int cursor);

    /**
     * Returns the event fired on layout with the cursor position point.
     */
    Event<Point>* getLayoutEvent();

    /**
     * Returns the event fired when the backspace/delete key is pressed.
     */
    Event<>* getBackspaceEvent();

    /**
     * Returns the event fired when the user cancels input (B button).
     */
    Event<>* getCancelEvent();

    /**
     * Returns the event fired when the user submits input (A/Start button).
     */
    Event<>* getSubmitEvent();

    /**
     * Returns the event fired when text is pasted from the clipboard.
     */
    Event<std::string>* getClipboardEvent();

    /**
     *  Refreshes the input label to reflect the current text or hint state.
     */
    void updateUI();

    bool isTranslucent() override;
    void onLayout() override;
    void show(const std::function<void(void)>& cb, bool animate, float animationDuration) override;

  private:
    std::string m_content;
    std::string m_hint;

    Event<Point> m_layoutEvent;
    Event<> m_backspaceEvent, m_cancelEvent, m_summitEvent;
    Event<std::string> m_clipboardEvent;
    Event<KeyState>::Subscription m_keyEvent;

    bool m_init = false;

    Animatable m_showOffset = 0;
    void offsetTick() { m_container->setTranslationY(m_showOffset); }

    BRLS_BIND(brls::Label, m_header, "brls/dialog/header");
    BRLS_BIND(brls::Label, m_label, "brls/dialog/label");
    BRLS_BIND(brls::Label, m_count, "brls/dialog/count");
    BRLS_BIND(brls::Box, m_container, "brls/dialog/container");
};
}