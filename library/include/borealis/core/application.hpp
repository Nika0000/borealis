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

#include <nanovg.h>
#include <tinyxml2.h>

#include <borealis/core/activity.hpp>
#include <borealis/core/audio.hpp>
#include <borealis/core/font.hpp>
#include <borealis/core/frame_context.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/notification_manager.hpp>
#include <borealis/core/platform.hpp>
#include <borealis/core/style.hpp>
#include <borealis/core/theme.hpp>
#include <borealis/core/view.hpp>
#include <borealis/views/label.hpp>
#include <deque>
#include <vector>

#ifdef __WINRT__
#ifdef main
#undef main
#endif
#define main SDL_main
#endif

namespace brls
{

// Input types for entire app
enum class InputType : uint8_t
{
    NONE    = 0,
    GAMEPAD = 1 << 0, // Gamepad or keyboard
    TOUCH   = 1 << 1, // Touch screen
    ALL     = GAMEPAD | TOUCH,
};

struct SafeAreaInsets
{
    float top, bottom;
    float left, right;
};

class DebugLayer;
class EditTextDialog;

typedef std::function<View*(void)> XMLViewCreator;

class Application
{
  public:
    friend class EditTextDialog;

    static inline uint32_t ORIGINAL_WINDOW_WIDTH  = 1280;
    static inline uint32_t ORIGINAL_WINDOW_HEIGHT = 720;

    /**
     * Inits the borealis application.
     * Returns true if it succeeded, false otherwise.
     */
    static bool init();

    /**
     * Creates the application window with the given title.
     * Must be called after calling init().
     */
    static void createWindow(const std::string& title);

    /**
     * Application main loop iteration.
     * Must be called in an infinite loop until it returns false.
     */
    static bool mainLoop();

    /**
     * Returns the platform abstraction layer instance.
     */
    static Platform* getPlatform();

    /**
     * Returns the shared Yoga layout configuration, creating it on first use.
     */
    static YGConfigRef getYogaConfig()
    {
        if (!yogaConfig)
        {
            yogaConfig = YGConfigNew();
            YGConfigSetUseWebDefaults(yogaConfig, true);
        }
        return yogaConfig;
    }

    /**
     * Returns the audio player instance used for UI sounds.
     */
    static AudioPlayer* getAudioPlayer();

    /**
     * Returns the notification manager used to display in-app notifications.
     */
    static NotificationManager* getNotificationManager();

    /**
     * Returns the NanoVG context used for rendering.
     */
    static NVGcontext* getNVGContext();

    /** Content area dimensions in logical pixels. */
    inline static float contentWidth, contentHeight;

    /** Current window dimensions in physical pixels. */
    inline static unsigned windowWidth, windowHeight;

    /** Current window position on screen. */
    inline static int windowXPos, windowYPos;

    /** Current safe area insets of the window. */
    inline static SafeAreaInsets windowSafeArea;

    /**
     * Called by the video context when the content window is resized
     */
    static void onWindowResized(int width, int height);

    /**
     * Do not call this function, use it internally.
     * Called when the video context is ready (to setup the initial content scaling).
     */
    static void setWindowSize(int width, int height);

    /**
     * Called by the video context when the content window is Repositioned
     */
    static void onWindowReposition(int xPos, int yPos);

    /**
     * Do not call this function, use it internally.
     * Called when the video context is ready (to setup the initial window position).
     */
    static void setWindowPosition(int xPos, int yPos);

    /**
     * Called by the video context when the safe area of the content window changes.
     * Provides the new safe area dimensions and margins.
     */
    static void onWindowSafeAreaChanged(SafeAreaInsets safeArea);

    /**
     * Do not call this function directly, it is used internally.
     * Called to set the initial safe area of the content window when the video context is ready.
     */
    static void setWindowSafeArea(SafeAreaInsets safeArea);

    /**
     * Returns the current activities stack.
     */
    static std::vector<Activity*> getActivitiesStack();

    /**
     * Pushes a view on this applications's view stack.
     *
     * The view will automatically be resized to take
     * the whole screen.
     *
     * The view will gain focus if applicable.
     *
     * The first activity to be pushed cannot be popped.
     * @param replace If true, the previous activity will be removed.
     */
    static void pushActivity(
        Activity* activity,
        bool replace                  = false,
        TransitionAnimation animation = TransitionAnimation::FADE //
    );

    /**
     * Pops the last pushed activity from the stack
     * and gives focus back where it was before.
     *
     * return false if no actifity to pop.
     */
    static bool popActivity(
        TransitionAnimation animation       = TransitionAnimation::FADE,
        const std::function<void(void)>& cb = [] {},
        bool free                           = true //
    );

    /**
     * Removes a specific activity from the stack.
     * Returns true if the activity was found and removed.
     * The first activity cannot be removed.
     */
    static bool deleteActivity(Activity* activity);

    /**
     * Gives the focus to the given view
     * or clears the focus if given nullptr.
     */
    static void giveFocus(View* view);

    inline static Style getStyle() { return brls::getStyle(); }

    /**
     * Returns the current theme.
     */
    static Theme getTheme();

    /**
     * Returns the current theme variant (light or dark).
     */
    static ThemeVariant getThemeVariant();

    /**
     * Returns the IME (Input Method Editor) manager for text input.
     */
    static ImeManager* getImeManager();

    /**
     * Loads a font from a given file and stores it in the font stash.
     * Returns true if the operation succeeded.
     */
    static bool loadFontFromFile(const std::string& fontName, const std::string& filePath);

    /**
     * Loads a font from a given memory buffer and stores it in the font stash.
     * Returns true if the operation succeeded.
     */
    static bool loadFontFromMemory(const std::string& fontName, void* data, size_t size, bool freeData);

    /**
     * Returns the nanovg handle to the given font name, or FONT_INVALID if
     * no such font is currently loaded.
     */
    static int getFont(const std::string& fontName);

    /**
     * Returns the nanovg handle for the default application font.
     */
    static int getDefaultFont();

    /**
     * Displays an in-app notification with the given text.
     * @param duration Display duration in milliseconds. 0 uses the default duration.
     */
    static void notify(const std::string& text, size_t duration = 0);

    /**
     * Processes a controller button press event.
     * @param repeating True if this is a repeated press from holding the button.
     */
    static void onControllerButtonPressed(enum ControllerButton button, bool repeating);

    /**
     * "Crashes" the app (displays a fullscreen CrashFrame)
     */
    static void crash(const std::string& text);

    /**
     * Requests application exit. The application will quit at the end of the current frame.
     */
    static void quit();

    /**
     * Blocks any and all user inputs
     */
    static void blockInputs(InputType type = InputType::ALL, bool muteSounds = false);

    /**
     * Unblocks inputs after a call to
     * blockInputs()
     */
    static void unblockInputs();

    /**
     * Returns true if user inputs of the given type are currently blocked.
     */
    static bool isInputBlocked(InputType type = InputType::ALL);

    /**
     * Checks if the application is currently in an interactive state.
     */
    static bool isInteractive();

    /**
     * Sets the application's foreground mode.
     *
     * When set to `true`, the application operates in the foreground, allowing
     * inputs, animations, and the renderer to be executed in the main loop.
     * When set to `false`, the application enters background mode, and these
     * processes are not executed in the main loop.
     */
    static void setForegroundMode(bool mode = true);

    /**
     * Sets the common footer text displayed at the bottom of the screen.
     */
    static void setCommonFooter(const std::string& footer);

    /**
     * Returns a pointer to the common footer text string.
     */
    static std::string* getCommonFooter();

    /** Window content scaling factor (DPI scaling). */
    inline static float windowScale;

    /**
     * Sets whether BUTTON_START will globally be used to close the application.
     */
    static void setGlobalQuit(bool enabled);

    /**
     * Enables or disables the FPS counter overlay.
     */
    static void setFPSStatus(bool enabled);

    /**
     * Returns whether the FPS counter overlay is enabled.
     */
    static bool getFPSStatus();

    /**
     * Returns the current frames per second.
     */
    static size_t getFPS();

    /**
     * Set the FPS limit
     * @param fps 0 to disable limit
     */
    static void setLimitedFPS(size_t fps);

    /**
     * Set the swap interval
     * Must be called after createWindow. On some platforms, it is impossible to set it back
     * to 0 after setting it to a non-zero values, so you can use
     * `VideoContext::swapInterval = 0;` (#include "borealis/core/video.hpp") before creating the window.
     *
     * Test results:
     *
     * ## Platforms that always vsync (interval: 1):
     * iOS18/GLES3/SDL
     * PS4/GLES2/SDL
     *
     * ## Platforms that only support enable and disable vsync (interval: 0,1):
     * Android9/GLES2/SDL
     * macOS13.0+/GL3.2/GLFW (will fixed to 120fps with vsync enabled, https://github.com/glfw/glfw/pull/2277)
     * Windows11/GL3.2/GLFW
     * Linux Wayland/GL3.2/GLFW
     *
     * ## Platforms that not support disable vsync (interval: 1,2,3,4):
     * Windows11/D3D11/GLFW
     * Switch/DEKO3D
     *
     * ## Platforms that support arbitrary values (interval: 0,1,2,3,4):
     * PsVita/GXM
     * PsVita/GLES2/SDL
     * macOS13.0+/GL3.2/SDL
     * Linux X11/GL3.2/GLFW
     * Linux X11/GL3.2/SDL
     * Linux Wayland/GL3.2/SDL
     * Switch/GL4.3/GLFW
     * Switch/GL4.3/SDL
     *
     * @param interval 0 to disable vsync
     */
    static void setSwapInterval(int interval);

    /**
     * If the value is set to true, the program will limit FPS to Application::DeactivatedFPS
     * after Application::DeactivatedTime milliseconds of inactivity.
     *
     * default is false;
     */
    static void setAutomaticDeactivation(bool value);

    /**
     * Returns whether automatic deactivation (FPS throttling on idle) is enabled.
     */
    static bool getAutomaticDeactivation();

    /**
     * Returns true if an active event has occurred recently.
     */
    static bool hasActiveEvent();

    /**
     * Signals an active event, resetting the inactivity timer.
     */
    static void setActiveEvent(bool value);

    /**
     * Sets the inactivity timeout before FPS is throttled.
     * @param millisecond Timeout in microseconds.
     */
    static void setDeactivatedTime(int millisecond);

    /**
     * Sets the target FPS when the application is in deactivated (idle) state.
     */
    static void setDeactivatedFPS(int value);

    /**
     * Returns the target FPS used during the deactivated (idle) state.
     */
    static int getDeactivatedFPS();

    /**
     * Returns the frame time corresponding to the deactivated FPS.
     */
    static double getDeactivatedFrameTime();

    /** Fired when focus changes between views. */
    static GenericEvent* getGlobalFocusChangeEvent();

    /** Fired when action hints should be refreshed. */
    static VoidEvent* getGlobalHintsUpdateEvent();

    /** Fired when the input type changes (e.g. gamepad to touch). */
    static Event<InputType>* getGlobalInputTypeChangeEvent();

    /** Fired every iteration of the main run loop. */
    static VoidEvent* getRunLoopEvent();

    /** Fired after each frame is rendered. */
    static VoidEvent* getPostFrameEvent();

    /** Fired when the application begins exiting. */
    static VoidEvent* getExitEvent();

    /** Fired after exit cleanup is complete. */
    static VoidEvent* getExitDoneEvent();

    /** Fired when the window size changes. */
    static VoidEvent* getWindowSizeChangedEvent();

    /** Fired when the window safe area insets change. */
    static Event<SafeAreaInsets>* getWindowSafeAreaChangedEvent();

    /** Fired after the window has been created and is ready. */
    static VoidEvent* getWindowCreationDoneEvent();

    /** Fired when a window close request is received. */
    static VoidEvent* getWindowShouldCloseEvent();

    /** Fired when the window gains or loses OS focus. */
    static Event<bool>* getWindowFocusChangedEvent();

    /**
     * Returns the currently focused view, or nullptr if nothing is focused.
     */
    static View* getCurrentFocus();

    /**
     * Returns the application window title.
     */
    static std::string getTitle();

    /**
     * Registers a view to be created from XML. You must give the name of the XML node as well
     * as a function that creates the view.
     *
     * If you need attributes, register them with the given functions in the view
     * class constructor directly. They will be called one by one after the view is instantiated.
     *
     * You should not add any children in the function, it is already taken care of.
     */
    static void registerXMLView(const std::string& name, XMLViewCreator creator);

    /**
     * Returns true if a view with the given XML tag name has been registered.
     */
    static bool XMLViewsRegisterContains(const std::string& name);

    /**
     * Returns the creator function for the given XML view tag name.
     */
    static XMLViewCreator getXMLViewCreator(const std::string& name);

    /**
     * Returns the current system locale.
     */
    static std::string getLocale();

    /**
     * Adds a view to the deletion pool to be freed at the end of the frame.
     */
    static void addToFreeQueue(View* view);

    /**
     * Returns the current input type.
     */
    inline static InputType getInputType() { return inputType; }

    inline static void enableDebuggingView(bool enable) { debuggingViewEnabled = enable; }

    inline static bool isDebuggingViewEnabled() { return debuggingViewEnabled; }

    /**
     * Sets whether confirm/cancel input keys should be swapped.
     */
    static void setSwapInputKeys(bool swap);

    /**
     * Returns true if confirm/cancel input keys are swapped.
     */
    inline static bool isSwapInputKeys() { return swapInputKeys; }

    /**
     * Enables or disables drawing of the cursor.
     */
    inline static void setDrawCoursor(bool draw) { drawCoursor = draw; }

    /**
     * Returns true if the cursor is being drawn.
     */
    inline static bool isDrawCursor() { return drawCoursor; }

    /**
     * Sets whether the half Joy-Con analog stick should be mapped to D-pad inputs.
     */
    inline static void setSwapHalfJoyconStickToDpad(bool swap) { swapHalfJoyconStickToDpad = swap; }

    /**
     * Returns true if half Joy-Con stick is mapped to D-pad.
     */
    inline static bool isSwapHalfJoyconStickToDpad() { return swapHalfJoyconStickToDpad; }

    /**
     * Attempts to deinitialize the first responder if the given view is the current one.
     */
    static void tryDeinitFirstResponder(View* view);

    /** Current touch input states for all active touch points. */
    inline static std::vector<TouchState> currentTouchState;

  private:
    inline static bool inited                    = false;
    inline static bool quitRequested             = false;
    inline static bool debuggingViewEnabled      = false;
    inline static bool swapInputKeys             = false;
    inline static bool drawCoursor               = false;
    inline static bool swapHalfJoyconStickToDpad = false;
    inline static bool interactive               = false;

    inline static Platform* platform = nullptr;
    inline static YGConfigRef yogaConfig = nullptr;

    inline static std::string title;

    inline static FontStash fontStash;

    inline static std::vector<Activity*> activitiesStack;
    inline static std::vector<View*> focusStack;
    inline static std::deque<View*> deletionPool;

    inline static View* currentFocus = nullptr;
    inline static MouseState currentMouseState;
    inline static NotificationManager* notificationManager;

    // Return true if input type was changed
    static bool setInputType(InputType type);

    inline static InputType inputType = InputType::GAMEPAD;

    inline static void processInput();
    inline static bool internalMainLoop();

    inline static void updateFPS();

    inline static std::vector<uint8_t> blockInputsMask = {};
    inline static bool muteSounds                      = false;

    inline static std::string commonFooter;
    inline static NVGcolor backgroundColor {};

    inline static bool globalQuitEnabled                = false;
    inline static ActionIdentifier gloablQuitIdentifier = ACTION_NONE;
    inline static bool globalFPSToggleEnabled           = false;
    inline static size_t globalFPS                      = 60;
    inline static Time limitedFrameTime                 = 0;
    inline static Time frameStartTime                   = 0;

    inline static bool deactivatedBehavior = false;
    inline static bool activeEvent         = false;
    inline static Time lastActiveTime      = 0;
    inline static int deactivatedFPS       = 5;       // FPS 5
    inline static int deactivatedTime      = 5000000; // 5s

    inline static View* repetitionOldFocus = nullptr;

    inline static GenericEvent globalFocusChangeEvent;
    inline static VoidEvent globalHintsUpdateEvent;
    inline static Event<InputType> globalInputTypeChangeEvent;
    inline static VoidEvent runLoopEvent;
    inline static VoidEvent postFrameEvent;
    inline static VoidEvent exitEvent;
    inline static VoidEvent exitDoneEvent;
    inline static VoidEvent windowSizeChangedEvent;
    inline static Event<SafeAreaInsets> windowSafeAreaChangeEvent;
    inline static VoidEvent windowCreationDoneEvent;
    inline static VoidEvent windowShouldCloseEvent;
    inline static Event<bool> windowFocusChangedEvent;

    inline static std::unordered_map<std::string, XMLViewCreator> xmlViewsRegister;

    static void navigate(FocusDirection direction, bool repeating);

    static void frame();
    static void clear();
    static void exit();

    /**
     * Handles actions for the currently focused view and
     * the given button
     * Returns true if at least one action has been fired
     */
    static bool handleAction(char button, bool repeating);

    static void registerBuiltInXMLViews();

    inline static DebugLayer* debugLayer = nullptr;
};

} // namespace brls
