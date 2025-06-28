#pragma once

#include "Clock.h"

namespace qe::Examples
{
    class CountdownTimer
    {
    public:
        CountdownTimer() = default;
        CountdownTimer(Clock::duration);
        bool IsRunning() const;
        bool IsExpired() const;

        void Start();
        void Stop();

        void SetDuration(Clock::duration countdownDuration);
        Clock::duration GetRemainingTime() const;

    private:
        Clock::duration m_duration = Clock::duration::zero();
        Clock::time_point m_startTime = Clock::now();
        bool m_running = false;
    };
}
