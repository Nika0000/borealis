/*
    Copyright 2023 xfangfang

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

#include <strings.h>

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_platform.hpp>
#include <unordered_map>

#if defined(IOS) || defined(TVOS)
#include <sys/utsname.h>

static bool isIPad()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    return strncmp(systemInfo.machine, "iPad", 4) == 0;
}
#endif

namespace brls
{

SDLPlatform::SDLPlatform()
{
#ifdef ANDROID
    // Enable Fullscreen on Android
    VideoContext::FULLSCREEN = true;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#elif defined(IOS) || defined(TVOS)
    // Enable Fullscreen on iOS
    VideoContext::FULLSCREEN = true;
    if (!isIPad())
    {
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    }
#elif defined(__APPLE__)
    // Same behavior as GLFW, change to the app's Resources directory if run in a ".app" bundle
    // Or to the executable's directory if run from other ways
    const char *base_path = SDL_GetBasePath();
    if (base_path)
    {
        chdir(base_path);
    }
#endif

    // Init sdl
    if (!SDL_Init(SDL_INIT_EVENTS))
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();

    // override local
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO)
    {
        int numLocales;
        SDL_Locale** locales = SDL_GetPreferredLocales(&numLocales);

        if (locales != nullptr && numLocales > 0)
        {
            std::unordered_map<std::string, std::string> localeMap = {
                { "zh_CN", LOCALE_ZH_HANS },
                { "zh_TW", LOCALE_ZH_HANT },
                { "ja_JP", LOCALE_JA },
                { "ko_KR", LOCALE_Ko },
                { "it_IT", LOCALE_IT },
                { "ru", LOCALE_RU }
            };


            for (int i = 0; i < numLocales; ++i)
            {
                std::string lang = std::string { locales[i]->language };
                if(locales[i]->country)
                {
                    lang += "_" + std::string { locales[i]->country };
                }

                if(localeMap.count(lang) > 0)
                {
                    this->locale = localeMap[lang];
                    Logger::info("Set app localeMM: {}", this->locale);
                    break;
                }
            }
        }

        if(this->locale.empty())
        {
            this->locale = LOCALE_EN_US;
            Logger::info("Set app locale to default: {}", this->locale);
        }

        SDL_free(locales);
    }
}

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    appTitle = windowTitle;
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new SDLImeManager(&this->otherEvent);
}

void SDLPlatform::restoreWindow()
{
    SDL_RestoreWindow(this->videoContext->getSDLWindow());
}

void SDLPlatform::setWindowAlwaysOnTop(bool enable)
{
    SDL_SetWindowAlwaysOnTop(this->videoContext->getSDLWindow(), enable ? true : false);
}

void SDLPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0) {
        SDL_SetWindowSize(this->videoContext->getSDLWindow(), windowWidth, windowHeight);
    }
}

void SDLPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        SDL_SetWindowMinimumSize(this->videoContext->getSDLWindow(), windowMinWidth, windowMinHeight);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxWidth > windowMinWidth && windowMaxHeight > windowMinHeight))
        SDL_SetWindowMaximumSize(this->videoContext->getSDLWindow(), windowMaxWidth, windowMaxHeight);
}

void SDLPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    SDL_SetWindowPosition(this->videoContext->getSDLWindow(), windowXPos, windowYPos);
}

void SDLPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        SDL_Window* win = this->videoContext->getSDLWindow();
        SDL_RestoreWindow(win);
        SDL_SetWindowSize(win, windowWidth, windowHeight);
        SDL_SetWindowPosition(win, windowXPos, windowYPos);
    }
}

void SDLPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    if (disable)
    {
        SDL_DisableScreenSaver();
    }
    else
    {
        SDL_EnableScreenSaver();
    }
}

bool SDLPlatform::isScreenDimmingDisabled()
{
    return !SDL_ScreenSaverEnabled();
}

void SDLPlatform::pasteToClipboard(const std::string& text)
{
    SDL_SetClipboardText(text.c_str());
}

std::string SDLPlatform::pasteFromClipboard()
{
    char *str = SDL_GetClipboardText();
    if (!str)
        return "";
    return std::string{str};
}

std::string SDLPlatform::getName()
{
    return "SDL";
}

bool SDLPlatform::processEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return false;
    }
    else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateKeyboardState(event->key);
    }
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseMotion(event->motion);
    }
    else if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseMotion(event->motion);
    }
    else if (event->type == SDL_EVENT_MOUSE_WHEEL)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateMouseWheel(event->wheel);
    }
    else if (event->type == SDL_EVENT_GAMEPAD_SENSOR_UPDATE)
    {
        auto* manager = this->inputManager;
        if (manager)
            manager->updateControllerSensorsUpdate(event->gsensor);
    }
#ifdef IOS
    else if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND)
    {
        brls::Application::getWindowFocusChangedEvent()->fire(false);
    }
    else if (event->type == SDL_EVENT_WILL_ENTER_FOREGROUND)
    {
        brls::Application::getWindowFocusChangedEvent()->fire(true);

    }
#endif
    else if (event->type != SDL_EVENT_POLL_SENTINEL)
    {
        // 其它没有处理的事件
        this->otherEvent.fire(event);
    }
    brls::Application::setActiveEvent(true);
    return true;
}

bool SDLPlatform::mainLoopIteration()
{
    SDL_Event event;
    bool hasEvent = false;
    while (SDL_PollEvent(&event))
    {
        if (!processEvent(&event))
        {
            return false;
        }
        hasEvent = true;
    }
    if (!hasEvent && !Application::hasActiveEvent())
    {
        if (SDL_WaitEventTimeout(&event, (int)(brls::Application::getDeactivatedFrameTime() * 1000))
            && !processEvent(&event))
        {
            return false;
        }
    }
    return true;
}

AudioPlayer* SDLPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

VideoContext* SDLPlatform::getVideoContext()
{
    return this->videoContext;
}

InputManager* SDLPlatform::getInputManager()
{
    return this->inputManager;
}

ImeManager* SDLPlatform::getImeManager() {
    return this->imeManager;
}

std::string SDLPlatform::getHomeDirectory(std::string appName) 
{
    auto name = appTitle;
    if (!appName.empty()) name = appName;

    return { SDL_GetPrefPath(nullptr, appName.c_str()) };
}

SDLPlatform::~SDLPlatform()
{
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->imeManager;
    delete this->videoContext;
}

} // namespace brls
