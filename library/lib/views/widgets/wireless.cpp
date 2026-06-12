/*
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

#include "borealis/views/widgets/wireless.hpp"

#include "borealis/core/thread.hpp"

#ifdef __SWITCH__
extern "C"
{
#include <switch/services/nifm.h>
}
#endif

namespace brls
{

WirelessWidget::WirelessWidget()
{
    m_platform = Application::getPlatform();
    if (!m_platform->canShowWirelessLevel())
        return;

    setSize(Size(44, 44));

    for (int i = 0; i < WIFI_LEVELS; i++)
    {
        m_wifi[i] = new Image();
        m_wifi[i]->setSize(Size(44, 44));
        m_wifi[i]->setScalingType(ImageScalingType::FIT);
        m_wifi[i]->detach();
        addView(m_wifi[i]);
    }

    m_ethernet = new Image();
    m_ethernet->setSize(Size(44, 44));
    m_ethernet->setScalingType(ImageScalingType::FIT);
    m_ethernet->detach();
    addView(m_ethernet);

    applyTheme(m_platform->getThemeVariant());
}

void WirelessWidget::applyTheme(ThemeVariant theme)
{
    const char* suffix = theme == ThemeVariant::LIGHT ? "light" : "dark";

    for (int i = 0; i < WIFI_LEVELS; i++)
        m_wifi[i]->setImageFromRes("img/sys/wifi_" + std::to_string(i) + "_" + suffix + ".png");

    m_ethernet->setImageFromRes(std::string("img/sys/ethernet_") + suffix + ".png");
}

void WirelessWidget::updateState()
{
    static Time time = 0;
    Time now         = getCPUTimeUsec();
    if ((now - time) > 5000000 || time == 0)
    {
        brls::Logger::verbose("hasWirelessConnection: {}; wifiLevel: {}", hasWirelessConnection, wifiLevel);
#ifdef ANDROID
        hasEthernetConnection = Application::getPlatform()->hasEthernetConnection();
        hasWirelessConnection = Application::getPlatform()->hasWirelessConnection();
        wifiLevel             = Application::getPlatform()->getWirelessLevel();
#else
        ASYNC_RETAIN
        brls::async(
            [ASYNC_TOKEN]()
            {
                ASYNC_RELEASE
#ifdef __SWITCH__
                // Reduce service calls
                // and fix support for emulator (Ryujinx) as it doesn't support :
                // nifmIsWirelessCommunicationEnabled() / nifmIsEthernetCommunicationEnabled().
                NifmInternetConnectionType type;
                u32 wifiSignal;
                NifmInternetConnectionStatus status;
                Result ret            = nifmGetInternetConnectionStatus(&type, &wifiSignal, &status);
                hasEthernetConnection = type == NifmInternetConnectionType_Ethernet;
                hasWirelessConnection = type == NifmInternetConnectionType_WiFi;
                if (ret != 0)
                {
                    hasEthernetConnection = false;
                    hasWirelessConnection = false;
                    wifiLevel             = 0;
                }
                else
                {
                    wifiLevel = (int) wifiSignal;
                }
#else
                hasEthernetConnection = Application::getPlatform()->hasEthernetConnection();
                hasWirelessConnection = Application::getPlatform()->hasWirelessConnection();
                wifiLevel             = Application::getPlatform()->getWirelessLevel();
#endif
            }
        );
#endif
        time = now;
    }
}

void WirelessWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    updateState();

    if (hasEthernetConnection)
    {
        for (int i = 0; i < WIFI_LEVELS; i++)
            m_wifi[i]->setVisibility(Visibility::GONE);
        m_ethernet->setVisibility(Visibility::VISIBLE);
    }
    else if (!hasWirelessConnection)
    {
        m_wifi[0]->setVisibility(Visibility::VISIBLE);
        for (int i = 1; i < WIFI_LEVELS; i++)
            m_wifi[i]->setVisibility(Visibility::GONE);
        m_ethernet->setVisibility(Visibility::GONE);
    }
    else
    {
        m_wifi[0]->setVisibility(Visibility::GONE);
        for (int i = 1; i < WIFI_LEVELS; i++)
        {
            m_wifi[i]->setVisibility(Visibility::VISIBLE);
            m_wifi[i]->setAlpha(i <= wifiLevel ? 1.0f : 0.2f);
        }
        m_ethernet->setVisibility(Visibility::GONE);
    }

    Box::draw(vg, x, y, width, height, style, ctx);
}

View* WirelessWidget::create() { return new WirelessWidget(); }

} // namespace brls
