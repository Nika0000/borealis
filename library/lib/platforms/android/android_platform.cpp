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

#include <SDL3/SDL.h>
#include <android/looper.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <jni.h>
#include <net/if.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/android/android_platform.hpp>

namespace brls
{

static bool getPlatformBool(const std::string& method, bool defaultValue = false)
{
    auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr)
        return defaultValue;

    jclass utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (utilsClass == nullptr)
        return defaultValue;
    jmethodID jmethod = env->GetStaticMethodID(utilsClass, method.c_str(), "()Z");
    if (jmethod == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return defaultValue;
    }
    jboolean value = env->CallStaticBooleanMethod(utilsClass, jmethod);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(utilsClass);
        return defaultValue;
    }
    env->DeleteLocalRef(utilsClass);
    return value;
}

static int getPlatformInt(const std::string& method, int defaultValue = 100)
{
    auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr)
        return defaultValue;

    jclass utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (utilsClass == nullptr)
        return defaultValue;
    jmethodID jmethod = env->GetStaticMethodID(utilsClass, method.c_str(), "()I");
    if (jmethod == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return defaultValue;
    }
    jint value = env->CallStaticIntMethod(utilsClass, jmethod);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(utilsClass);
        return defaultValue;
    }
    env->DeleteLocalRef(utilsClass);
    return value;
}

bool AndroidPlatform::canShowBatteryLevel() { return getPlatformBool("isBatterySupported"); }

int AndroidPlatform::getBatteryLevel() { return getPlatformInt("getBatteryLevel"); }

bool AndroidPlatform::isBatteryCharging() { return getPlatformBool("isBatteryCharging"); }

bool AndroidPlatform::canShowWirelessLevel() { return getPlatformBool("isWifiSupported"); }

bool AndroidPlatform::hasWirelessConnection() { return getPlatformBool("isWifiConnected"); }

int AndroidPlatform::getWirelessLevel()
{
    int level = getPlatformInt("getWifiSignalStrength");
    if (level >= -60)
    {
        return 3;
    }
    else if (level >= -70)
    {
        return 2;
    }
    else if (level >= -80)
    {
        return 1;
    }
    return 0;
}

bool AndroidPlatform::hasEthernetConnection() { return getPlatformBool("isEthernetConnected"); }

std::string AndroidPlatform::getIpAddress()
{
    std::string ipaddr = "-";
    int sock;
    struct ifconf conf;
    char data[4096];
    struct ifreq* ifr;
    struct sockaddr_in* sock_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return "-";
    }

    conf.ifc_len = sizeof(data);
    conf.ifc_buf = (caddr_t) data;
    if (ioctl(sock, SIOCGIFCONF, &conf) < 0)
    {
        close(sock);
        return "-";
    }

    ifr = (struct ifreq*) data;
    while ((char*) ifr < data + conf.ifc_len)
    {
        if (ifr->ifr_addr.sa_family == AF_INET)
        {
            sock_addr          = (struct sockaddr_in*) &ifr->ifr_addr;
            const char* result = inet_ntoa(sock_addr->sin_addr);
            if (result)
                ipaddr = std::string { result };
        }
        ifr = (struct ifreq*) ((char*) ifr + sizeof(*ifr));
    }
    close(sock);

    return ipaddr;
}

std::string AndroidPlatform::getDnsServer() { return "-"; }

void AndroidPlatform::openBrowser(std::string url)
{
    auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr)
        return;

    jclass utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (utilsClass == nullptr)
    {
        return;
    }

    jmethodID openBrowserMethod = env->GetStaticMethodID(utilsClass, "openBrowser", "(Ljava/lang/String;)V");
    if (openBrowserMethod == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return;
    }

    auto jurl = env->NewStringUTF(url.c_str());
    env->CallStaticVoidMethod(utilsClass, openBrowserMethod, jurl);

    env->DeleteLocalRef(utilsClass);
    env->DeleteLocalRef(jurl);
}

float AndroidPlatform::getBacklightBrightness()
{
    auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr)
        return 0.0f;

    jclass utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (utilsClass == nullptr)
        return 0.0f;
    auto activity = (jobject) SDL_GetAndroidActivity();
    if (activity == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return 0.0f;
    }
    jmethodID jmethod = env->GetStaticMethodID(utilsClass, "getAppScreenBrightness", "(Landroid/app/Activity;)F");
    if (jmethod == nullptr)
    {
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(utilsClass);
        return 0.0f;
    }
    jfloat value = env->CallStaticFloatMethod(utilsClass, jmethod, activity);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(utilsClass);
        return 0.0f;
    }
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(utilsClass);
    return value;
}

void AndroidPlatform::setBacklightBrightness(float brightness)
{
    auto env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr)
        return;

    jclass utilsClass = env->FindClass("org/libsdl/app/PlatformUtils");
    if (utilsClass == nullptr)
    {
        return;
    }

    jmethodID method = env->GetStaticMethodID(utilsClass, "setAppScreenBrightness", "(Landroid/app/Activity;F)V");
    if (method == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return;
    }

    auto activity = (jobject) SDL_GetAndroidActivity();
    if (activity == nullptr)
    {
        env->DeleteLocalRef(utilsClass);
        return;
    }
    env->CallStaticVoidMethod(utilsClass, method, activity, brightness);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
    }
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(utilsClass);
}

bool AndroidPlatform::canSetBacklightBrightness() { return true; }

using PostFrameCallback64Fn = void (*)(AChoreographer*, void (*)(int64_t, void*), void*);

void AndroidPlatform::postFrameCallback()
{
    static auto postFrameCallback64 = reinterpret_cast<PostFrameCallback64Fn>(dlsym(RTLD_DEFAULT, "AChoreographer_postFrameCallback64"));

    if (postFrameCallback64)
        postFrameCallback64(m_choreographer, choreographerCallback64, this);
    else
        AChoreographer_postFrameCallback(m_choreographer, choreographerCallback, this);
}

void AndroidPlatform::choreographerCallback(long, void* data)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    static_cast<AndroidPlatform*>(data)->onFrame(static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec);
}

void AndroidPlatform::choreographerCallback64(int64_t frameTimeNanos, void* data)
{
    static_cast<AndroidPlatform*>(data)->onFrame(frameTimeNanos);
}

void AndroidPlatform::onFrame(int64_t frameTimeNanos)
{
    if (!m_running)
        return;

    if (VideoContext::swapInterval != 0)
        Application::setSwapInterval(0);

    // Track the display refresh interval from consecutive vsync timestamps.
    if (m_lastFrameTimeNanos > 0)
    {
        const int64_t delta = frameTimeNanos - m_lastFrameTimeNanos;
        // Ignore outliers (missed callbacks, resume after pause): 4ms..100ms is 10..250Hz.
        if (delta > 4000000 && delta < 100000000)
            m_vsyncPeriodNanos = (m_vsyncPeriodNanos == 0) ? delta : (m_vsyncPeriodNanos * 7 + delta) / 8;
    }
    m_lastFrameTimeNanos = frameTimeNanos;

    const Time limitedFrameTime = Application::getLimitedFrameTime(); // microseconds
    int64_t minDeltaNanos       = static_cast<int64_t>(limitedFrameTime) * 1000;

    if (minDeltaNanos > 0 && m_vsyncPeriodNanos > 0 && minDeltaNanos <= m_vsyncPeriodNanos + m_vsyncPeriodNanos / 8)
        minDeltaNanos = 0;

    if (minDeltaNanos > 0 && m_nextDeadlineNanos >= 0)
    {
        // Accept a vsync that lands within half a refresh period before the deadline:
        // waiting for the next one would overshoot the target interval by more than
        // rendering now undershoots it.
        const int64_t slackNanos = m_vsyncPeriodNanos > 0 ? m_vsyncPeriodNanos / 2 : 0;
        if (frameTimeNanos + slackNanos < m_nextDeadlineNanos)
        {
            postFrameCallback();
            return;
        }
    }

    if (minDeltaNanos > 0)
    {
        if (m_nextDeadlineNanos >= 0 && frameTimeNanos - m_nextDeadlineNanos < minDeltaNanos)
            m_nextDeadlineNanos += minDeltaNanos;
        else
            m_nextDeadlineNanos = frameTimeNanos + minDeltaNanos;
    }
    else
    {
        m_nextDeadlineNanos = -1;
    }

    if ((*m_runLoopImpl)())
        postFrameCallback();
    else
        m_running = false;
}

bool AndroidPlatform::runLoop(const std::function<bool()>& runLoopImpl)
{
    if (!ALooper_forThread())
        ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

    m_choreographer = AChoreographer_getInstance();
    if (!m_choreographer)
    {
        Logger::warning("AndroidPlatform: AChoreographer unavailable, falling back to plain loop");
        while (runLoopImpl())
        {
        }
        return false;
    }

    m_runLoopImpl = &runLoopImpl;
    m_running     = true;

    postFrameCallback();

    // Block until the next vsync callback (or any fd event on this looper).
    while (m_running)
        ALooper_pollOnce(-1, nullptr, nullptr, nullptr);

    return false;
}

} // namespace brls
