/*
    Copyright 2021 natinusala
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

#import <CoreWLAN/CoreWLAN.h>

#import <borealis/core/logger.hpp>
#import <borealis/platforms/desktop/desktop_platform.hpp>
#include <functional>

#if defined(BOREALIS_USE_METAL)
#import <AppKit/AppKit.h>
#import <QuartzCore/CADisplayLink.h>

API_AVAILABLE(macos(14.0))
@interface BRLSDarwinDisplayLinkTarget : NSObject
{
  @public
    std::function<bool()> runLoopImpl;
    BOOL isDispatching;
}
@end

@implementation BRLSDarwinDisplayLinkTarget
- (void)displayLinkDidFire:(CADisplayLink*)sender
{
    if (isDispatching)
        return;

    @autoreleasepool
    {
        isDispatching    = YES;
        bool keepRunning = runLoopImpl();
        isDispatching    = NO;

        if (!keepRunning)
        {
            [sender invalidate];
            [NSApp stop:nil];
        }
    }
}
@end
#endif

namespace brls
{

#if defined(BOREALIS_USE_METAL)
namespace
{
    NSWindow* applicationWindow()
    {
        if (NSApp.mainWindow != nil)
            return NSApp.mainWindow;
        if (NSApp.keyWindow != nil)
            return NSApp.keyWindow;
        for (NSWindow* candidate in NSApp.windows)
            if (candidate.visible)
                return candidate;
        return nil;
    }
} // namespace
#endif

// Interface method, fetching the current connection info.
int darwin_wlan_quality()
{
    @autoreleasepool
    {
        CWWiFiClient* Client          = CWWiFiClient.sharedWiFiClient;
        CWInterface* currentInterface = Client.interface;
        if (![currentInterface powerOn])
        {
            return -1;
        }
        if (![currentInterface serviceActive])
        {
            return 0;
        }
        int rssi = [currentInterface rssiValue];
        if (rssi > -50)
            return 3;
        if (rssi > -80)
            return 2;
        return 1;
    }
}

bool darwin_runloop(const std::function<bool()>& runLoopImpl)
{
    @autoreleasepool
    {
#if defined(BOREALIS_USE_METAL)
        NSWindow* window = @available(macOS 14.0, *) ? applicationWindow() : nil;
        if (window != nil)
        {
            auto* target        = [[BRLSDarwinDisplayLinkTarget alloc] init];
            target->runLoopImpl = runLoopImpl;

            CADisplayLink* displayLink = [window displayLinkWithTarget:target selector:@selector(displayLinkDidFire:)];
            if (displayLink != nil)
            {
                NSInteger frameRate = window.screen.maximumFramesPerSecond;
                if (frameRate <= 0)
                    frameRate = 60;
                displayLink.preferredFrameRateRange = CAFrameRateRangeMake(
                    static_cast<float>((frameRate * 2) / 3), static_cast<float>(frameRate), static_cast<float>(frameRate)
                );
                [displayLink addToRunLoop:NSRunLoop.currentRunLoop forMode:NSDefaultRunLoopMode];

                Logger::debug("darwin: Metal UI requested {} Hz CADisplayLink", frameRate);
                [NSApp run];
                [displayLink invalidate];
#if !__has_feature(objc_arc)
                [target release];
#endif
                return false;
            }

#if !__has_feature(objc_arc)
            [target release];
#endif
            Logger::error("darwin: failed to create Metal CADisplayLink");
        }
#endif
        return runLoopImpl();
    }
}

}