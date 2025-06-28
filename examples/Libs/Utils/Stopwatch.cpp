#include "Stopwatch.h"
#include "Defensive.h"

namespace qe::Examples
{
    bool Stopwatch::IsRunning() const
    {
        return m_pauseTime == Clock::time_point{};
    }

    void Stopwatch::Pause()
    {
        ReturnUnless(IsRunning());
        m_pauseTime = Clock::now();
        m_accumulated += m_pauseTime - m_resumeTime;
    }

    void Stopwatch::Resume()
    {
        ReturnIf(IsRunning());
        m_resumeTime = Clock::now();
        m_pauseTime = {};
    }

    Clock::duration Stopwatch::Measure() const
    {
        auto elapsed = m_accumulated;
        if (IsRunning())
        {
            elapsed += Clock::now() - m_resumeTime;
        }
        return elapsed;
    }

    Clock::duration Stopwatch::Reset()
    {
        auto now = Clock::now();
        auto elapsed = m_accumulated;
        if (IsRunning())
        {
            elapsed += now - m_resumeTime;
        }
        m_resumeTime = now;
        m_pauseTime = {};
        m_accumulated = {};
        return elapsed;
    }
}
