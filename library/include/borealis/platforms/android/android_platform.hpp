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

#pragma once

#include <android/choreographer.h>

#include <atomic>
#include <borealis/platforms/sdl/sdl_platform.hpp>
#include <cstdint>
#include <functional>

namespace brls
{

class AndroidPlatform : public SDLPlatform
{
  public:
    bool canShowBatteryLevel() override;
    bool canShowWirelessLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool hasWirelessConnection() override;
    int getWirelessLevel() override;
    bool hasEthernetConnection() override;
    std::string getIpAddress() override;
    std::string getDnsServer() override;
    void openBrowser(std::string url) override;
    float getBacklightBrightness() override;
    void setBacklightBrightness(float brightness) override;
    bool canSetBacklightBrightness() override;

    bool runLoop(const std::function<bool()>& runLoopImpl) override;

  private:
    static void choreographerCallback(long frameTimeNanos, void* data);
    static void choreographerCallback64(int64_t frameTimeNanos, void* data);

    void postFrameCallback();
    void onFrame(int64_t frameTimeNanos);

    std::atomic<bool> m_running { false };

    AChoreographer* m_choreographer            = nullptr;
    const std::function<bool()>* m_runLoopImpl = nullptr;

    int64_t m_nextDeadlineNanos  = -1;
    int64_t m_lastFrameTimeNanos = 0;
    int64_t m_vsyncPeriodNanos   = 0;
};

} // namespace brls
