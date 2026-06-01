#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/edit_text_dialog.hpp>

#ifdef __PSV__
#define EDIT_TEXT_DIALOG_POP_ANIMATION TransitionAnimation::NONE
#define EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT false
#else
#define EDIT_TEXT_DIALOG_POP_ANIMATION TransitionAnimation::FADE
#define EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT true
#endif

#if defined(__PSV__) || defined(IOS) || defined(ANDROID)
#include <borealis/platforms/sdl/sdl_video.hpp>
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
    inflateFromXMLString(editTextDialogXML);

    registerAction(
        "hints/paste"_i18n,
        BUTTON_X,
        [this](...)
        {
            this->m_clipboardEvent.fire(Application::getPlatform()->pasteFromClipboard());
            return true;
        }
    );

    // submit text
    registerAction(
        "hints/ok"_i18n,
        BUTTON_A,
        [this](...)
        {
            Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this]() { this->m_summitEvent.fire(); });
            return true;
        }
    );

    registerAction(
        "hints/ok"_i18n,
        BUTTON_START,
        [this](...)
        {
            Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this]() { this->m_summitEvent.fire(); });
            return true;
        },
        true
    );

    m_keyEvent = Application::getPlatform()->getInputManager()->getKeyboardKeyStateChanged()->subscribe(
        [this](const KeyState& state)
        {
            if (!state.pressed)
                return;

            switch (state.key)
            {
                case BRLS_KBD_KEY_BACKSPACE:
                case BRLS_KBD_KEY_DELETE: // This is to ensure that the delete operation of the PSV ime will not be ignored
                    this->m_backspaceEvent.fire();
                    break;
                case BRLS_KBD_KEY_V:
#ifdef __APPLE__
                    if (state.mods & (BRLS_KBD_MODIFIER_CTRL | BRLS_KBD_MODIFIER_META))
                    {
#else
                    if (state.mods & BRLS_KBD_MODIFIER_CTRL)
                    {
#endif
                        this->m_clipboardEvent.fire(Application::getPlatform()->pasteFromClipboard());
                    }
                    break;
                default:
                    break;
            }
        }
    );

    registerAction(
        "hints/delete"_i18n,
        BUTTON_BACK,
        [this](...)
        {
            this->m_backspaceEvent.fire();
            return true;
        },
        true,
        true
    );

    // cancel input
    registerAction(
        "hints/back"_i18n,
        BUTTON_B,
        [this](...)
        {
            Application::popActivity(EDIT_TEXT_DIALOG_POP_ANIMATION, [this]() { this->m_cancelEvent.fire(); });
            return true;
        }
    );

#if defined(__PSV__) || defined(IOS) || defined(ANDROID)
    // After turning off the on-screen keyboard, tap on the input area to reopen
    addGestureRecognizer(new brls::TapGestureRecognizer(
        [](brls::TapGestureStatus status, brls::Sound*)
        {
            if (status.state == brls::GestureState::END)
            {
                auto* videoContext = (SDLVideoContext*) Application::getPlatform()->getVideoContext();
                auto* window       = videoContext->getSDLWindow();
                if (!SDL_ScreenKeyboardShown(window))
                {
                    SDL_StopTextInput(window);
                    SDL_StartTextInput(window);
                }
                Application::setInputType(InputType::GAMEPAD);
            }
        }
    ));
    // Force to set to gamepad mode to avoid missing the `Enter` keyboard event
    Application::setInputType(InputType::GAMEPAD);
#endif

    this->m_init = true;
}

void EditTextDialog::show(const std::function<void(void)>& cb, bool animate, float animationDuration)
{
    if (animate)
    {
        m_container->setTranslationY(-150.0f);

        m_showOffset.stop();
        m_showOffset.reset(-150.0f);
        m_showOffset.addStep(0.0f, animationDuration, brls::EasingFunction::quadraticOut);
        m_showOffset.setTickCallback([this] { this->offsetTick(); });
        m_showOffset.start();
    }

    Box::show(cb, animate, animationDuration);
}

EditTextDialog::~EditTextDialog() { Application::getPlatform()->getInputManager()->getKeyboardKeyStateChanged()->unsubscribe(m_keyEvent); }

void EditTextDialog::open() { Application::pushActivity(new Activity(this)); }

void EditTextDialog::setText(const std::string& value)
{
    m_content = value;
    updateUI();
}

void EditTextDialog::setHeaderText(const std::string& value)
{
    if (!value.empty())
    {
        m_header->setText(value);
        m_header->setVisibility(Visibility::VISIBLE);
        return;
    }

    m_header->setVisibility(Visibility::GONE);
}

void EditTextDialog::setHintText(const std::string& value)
{
    if (value.empty())
    {
        m_hint = "hints/input"_i18n;
    }
    else
    {
        m_hint = value;
    }

    updateUI();
}

void EditTextDialog::setCountText(const std::string& value) { m_count->setText(value); }

bool EditTextDialog::isTranslucent() { return EDIT_TEXT_DIALOG_BACKGROUND_TRANSLUCENT; }

void EditTextDialog::onLayout()
{
    if (!m_init)
        return;

    m_layoutEvent.fire(Point { m_label->getX(), m_label->getY() + m_label->getHeight() });
}

Event<Point>* EditTextDialog::getLayoutEvent() { return &m_layoutEvent; }

Event<>* EditTextDialog::getBackspaceEvent() { return &m_backspaceEvent; }

Event<>* EditTextDialog::getCancelEvent() { return &m_cancelEvent; }

Event<>* EditTextDialog::getSubmitEvent() { return &m_summitEvent; }

Event<std::string>* EditTextDialog::getClipboardEvent() { return &m_clipboardEvent; }

void EditTextDialog::updateUI()
{
    if (m_content.empty())
    {
        m_label->setTextColor(Application::getTheme().getColor("brls/text_disabled"));
        m_label->setText(m_hint);
        m_label->setCursor((int) CursorPosition::START);
    }
    else
    {
        m_label->setTextColor(Application::getTheme().getColor("brls/text"));
        m_label->setText(m_content);
    }
}

void EditTextDialog::setCursor(int cursor) { m_label->setCursor(cursor); }
}
