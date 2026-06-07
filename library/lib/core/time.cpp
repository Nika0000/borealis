/*
    Copyright 2021 natinusala

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

#include <borealis/core/time.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef _WIN32
#include <timeapi.h>

#elif !defined(__SWITCH__)
#include <time.h>
#endif

namespace brls
{

// Spin margin: microseconds reserved for the final spin-wait.
// Must be large enough to absorb OS timer jitter so we never overshoot the deadline.
// 2ms is safe for all platforms; the spin uses pause intrinsics to minimize power.
static constexpr Time SPIN_MARGIN_USEC = 2000;

void waitUntilUsec(Time targetUsec)
{
    Time now = getCPUTimeUsec();
    if (now >= targetUsec)
        return;

    Time remaining = targetUsec - now;

    // OS sleep for the bulk — only if there's enough time beyond the spin margin
    if (remaining > SPIN_MARGIN_USEC)
    {
#ifdef _WIN32
        static HANDLE s_timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        Time sleepUsec = remaining - SPIN_MARGIN_USEC;

        if (s_timer)
        {
            LARGE_INTEGER dueTime;
            dueTime.QuadPart = -(static_cast<LONGLONG>(sleepUsec) * 10);
            SetWaitableTimer(s_timer, &dueTime, 0, NULL, NULL, FALSE);
            WaitForSingleObject(s_timer, INFINITE);
        }
        else
        {
            timeBeginPeriod(1);
            Sleep(static_cast<DWORD>(sleepUsec / 1000));
            timeEndPeriod(1);
        }
#elif defined(__SWITCH__)
        extern void svcSleepThread(long long ns);
        svcSleepThread(static_cast<long long>(remaining - SPIN_MARGIN_USEC) * 1000);
#else
        Time sleepUsec = remaining - SPIN_MARGIN_USEC;
        struct timespec ts;
        ts.tv_sec  = sleepUsec / 1000000;
        ts.tv_nsec = (sleepUsec % 1000000) * 1000;
        nanosleep(&ts, NULL);
#endif
    }

    // Spin-wait the final ~2ms with pause intrinsics (low power, no scheduler involvement)
    while (getCPUTimeUsec() < targetUsec)
    {

#if defined(_WIN32)
        YieldProcessor();
#elif defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("yield");
#endif
    }
}

void Ticking::updateTickings()
{
    // Update time
    static Time previousTime = 0;

    Time currentTime = getCPUTimeUsec() / 1000;
    Time delta       = previousTime == 0 ? 0 : currentTime - previousTime;

    previousTime = currentTime;

    // Update every running ticking, kill them and execute cb if they are finished
    // We have to clone the running tickings list to avoid altering it while
    // in the for loop (so if another ticking is started in a callback or during onUpdate())
    std::vector<Ticking*> tickings(Ticking::runningTickings);

    for (Ticking* ticking : tickings)
    {
        bool run = ticking->onUpdate(delta);

        ticking->m_tickCallback();

        if (!run)
            ticking->stop(true); // will remove the ticking from Ticking::runningTickings
    }
}

void Ticking::start()
{
    if (m_running)
        return;

    Ticking::runningTickings.push_back(this);

    m_running = true;

    onStart();
}

void Ticking::stop() { stop(false); }

void Ticking::stop(bool finished)
{
    if (!m_running)
        return;

    for (size_t i = 0; i < Ticking::runningTickings.size(); i++)
    {
        if (Ticking::runningTickings[i] == this)
        {
            Ticking::runningTickings.erase(Ticking::runningTickings.begin() + i);
            break;
        }
    }

    m_running = false;

    onStop();

    m_endCallback(finished);
}

void Ticking::setEndCallback(const TickingEndCallback& endCallback) { m_endCallback = endCallback; }

void Ticking::setTickCallback(const TickingTickCallback& tickCallback) { m_tickCallback = tickCallback; }

bool Ticking::isRunning() const { return m_running; }

Ticking::~Ticking() { stop(); }

void FiniteTicking::rewind() { onRewind(); }

void FiniteTicking::reset()
{
    stop();
    onReset();
}

} // namespace brls
