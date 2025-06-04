#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/edit_text_dialog.hpp>

#ifdef __PSV__
#define EDIT_TEXT_DIALOG_POP_ANIMATION TransitionAnimation::NONE
#define EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT false
#include <SDL3/SDL.h>

#include "borealis/platforms/sdl/sdl_video.hpp"

#else
#define EDIT_TEXT_DIALOG_POP_ANIMATION TransitionAnimation::FADE
#define EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT true
#endif

namespace brls
{

const std::string editTextDialogXML = R"xml(
        <brls:Box
            width="auto"
            height="auto"
            axis="column"
            justifyContent="flexStart"
            alignItems="center"
            focusable="true"
            hideHighlight="true"
            backgroundColor="@theme/brls/backdrop">
    
            <brls:Box
                id="brls/dialog/container"
                alignItems="center"
                width="@style/brls/dialog/width")xml"
#ifdef __PSV__
                                      R"xml(marginTop="30")xml"
#else
                                      R"xml(marginTop="50")xml"
#endif
                                      R"xml(axis="column">
                
                <brls:Label
                    id="brls/dialog/header"
                    visibility="gone"
                    fontSize="@style/brls/dialog/fontSize"
                    marginBottom="20"
                    textColor="#FFFFFF"/>

                <brls:Box
                    width="auto"
                    height="auto"
                    cornerRadius="@style/brls/dialog/cornerRadius"
                    alignItems="flexEnd"
                    axis="column"
                    padding="@style/brls/dialog/paddingContent"
                    backgroundColor="@theme/brls/background">

                    <brls:Label
                        id="brls/dialog/label"
                        grow="1"
                        width="100%"
                        cursor="-1"
                        minHeight="30"
                        autoAnimate="false"
                        verticalAlign="top"/>

                    <brls:Label
                        id="brls/dialog/count"
                        horizontalAlign="right"
                        fontSize="18"
                        textColor="@theme/brls/text_disabled"
                        marginRight="16"
                        marginBottom="10"/>

                    <brls:Hints
                        allowAButtonTouch="true"
                        forceShown="true"
                        addBaseAction="false"
                        width="auto"
                        height="auto"/>
                </brls:Box>
            </brls:Box>
        </brls:Box>
        )xml";
EditTextDialog::EditTextDialog()
{
    this->inflateFromXMLString(editTextDialogXML);

    // submit text
    this->registerAction(
        "hints/ok"_i18n, BUTTON_A, [this](...)
        {
                Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this](){
                        this->summitEvent.fire();
                    });
                return true; });
    this->registerAction(
        "hints/ok"_i18n, BUTTON_START, [this](...)
        {
                Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this](){
                        this->summitEvent.fire();
                    });
                return true; }, true);

    keyEvent = Application::getPlatform()->getInputManager()->getKeyboardKeyStateChanged()->subscribe([this](const KeyState& state)
        {
            if (!state.pressed) return;

            switch (state.key) {
                case BRLS_KBD_KEY_BACKSPACE:
                case BRLS_KBD_KEY_DELETE: // This is to ensure that the delete operation of the PSV ime will not be ignored
                    this->backspaceEvent.fire();
                    break;
                case BRLS_KBD_KEY_V:
#ifdef __APPLE__
                    if (state.mods & (BRLS_KBD_MODIFIER_CTRL | BRLS_KBD_MODIFIER_META)) {
#else
                    if (state.mods & BRLS_KBD_MODIFIER_CTRL) {
#endif
                        this->clipboardEvent.fire(Application::getPlatform()->pasteFromClipboard());
                    }
                    break;
                default:
                    break;
            } });

    this->registerAction("hints/delete"_i18n, BUTTON_BACK, [this](...)
        {
                this->backspaceEvent.fire();
                return true; }, true, true);

    // cancel input
    this->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](...)
        {
                Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this](){
                        this->cancelEvent.fire();
                    });
                return true; });

#ifdef __PSV__
    // After turning off the on-screen keyboard, tap on the input area to reopen
    this->addGestureRecognizer(new brls::TapGestureRecognizer([](brls::TapGestureStatus status, brls::Sound*)
        {
            if (status.state == brls::GestureState::END) {
                auto* videoContext = (SDLVideoContext*)Application::getPlatform()->getVideoContext();
                if (!SDL_IsScreenKeyboardShown(videoContext->getSDLWindow()))
                {
                    SDL_StopTextInput();
                    SDL_StartTextInput();
                }
                Application::setInputType(InputType::GAMEPAD);
            } }));
    // Force to set to gamepad mode to avoid missing the `Enter` keyboard event
    Application::setInputType(InputType::GAMEPAD);
#endif

    this->init = true;
}

void EditTextDialog::show(std::function<void(void)> cb, bool animate, float animationDuration)
{
    if (animate)
    {
        container->setTranslationY(-150.0f);

        showOffset.stop();
        showOffset.reset(-150.0f);
        showOffset.addStep(0.0f, animationDuration, brls::EasingFunction::quadraticOut);
        showOffset.setTickCallback([this]
            { this->offsetTick(); });
        showOffset.start();
    }

    Box::show(cb, animate, animationDuration);
}

EditTextDialog::~EditTextDialog()
{
    Application::getPlatform()->getInputManager()->getKeyboardKeyStateChanged()->unsubscribe(keyEvent);
}

void EditTextDialog::open()
{
    Application::pushActivity(new Activity(this));
}

void EditTextDialog::setText(const std::string& value)
{
    this->content = value;
    this->updateUI();
}

void EditTextDialog::setHeaderText(const std::string& value)
{
    if (!value.empty())
    {
        this->header->setText(value);
        this->header->setVisibility(Visibility::VISIBLE);
        return;
    }

    this->header->setVisibility(Visibility::GONE);
}

void EditTextDialog::setHintText(const std::string& value)
{
    if (value.empty())
    {
        this->hint = "hints/input"_i18n;
    }
    else
    {
        this->hint = value;
    }
    this->updateUI();
}

void EditTextDialog::setCountText(const std::string& value)
{
    this->count->setText(value);
}

bool EditTextDialog::isTranslucent()
{
    return EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT;
}

void EditTextDialog::onLayout()
{
    if (!init)
        return;
    this->layoutEvent.fire(Point { this->label->getX(),
        this->label->getY() + this->label->getHeight() });
}

Event<Point>* EditTextDialog::getLayoutEvent()
{
    return &this->layoutEvent;
}

Event<>* EditTextDialog::getBackspaceEvent()
{
    return &this->backspaceEvent;
}

Event<>* EditTextDialog::getCancelEvent()
{
    return &this->cancelEvent;
}

Event<>* EditTextDialog::getSubmitEvent()
{
    return &this->summitEvent;
}

Event<std::string>* EditTextDialog::getClipboardEvent()
{
    return &this->clipboardEvent;
}

void EditTextDialog::updateUI()
{
    if (content.empty())
    {
        label->setTextColor(Application::getTheme().getColor("brls/text_disabled"));
        label->setText(hint);
        label->setCursor((int)CursorPosition::START);
    }
    else
    {
        label->setTextColor(Application::getTheme().getColor("brls/text"));
        label->setText(content);
    }
}

void EditTextDialog::setCursor(int cursor)
{
    label->setCursor(cursor);
}
}
