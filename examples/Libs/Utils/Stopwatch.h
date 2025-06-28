#pragma once

#include "Clock.h"

namespace qe::Examples
{
    class Stopwatch
    {
    public:
        bool IsRunning() const;
        void Pause();
        void Resume();

        Clock::duration Measure() const;
        Clock::duration Reset();

    private:
        Clock::time_point m_resumeTime = Clock::now();
        Clock::time_point m_pauseTime = {};
        Clock::duration m_accumulated = {};
    };
}
