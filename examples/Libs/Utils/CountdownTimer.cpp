#include "CountdownTimer.h"
#include "Defensive.h"

namespace qe::Examples
{
CountdownTimer::CountdownTimer(Clock::duration countdownDuration)
{
    SetDuration(countdownDuration);
    Start();
}

bool CountdownTimer::IsRunning() const
    {
        return m_running;
    }

    bool CountdownTimer::IsExpired() const
    {
        return GetRemainingTime() <= Clock::duration::zero();
    }

    void CountdownTimer::Start()
    {
        m_startTime = Clock::now();
        m_running = true;
    }

    void CountdownTimer::Stop()
    {
        m_running = false;
    }

    void CountdownTimer::SetDuration(Clock::duration countdownDuration)
    {
        m_duration = countdownDuration;
    }

    Clock::duration CountdownTimer::GetRemainingTime() const
    {
        ReturnIf(!m_running, Clock::duration::zero());
        return m_duration - (Clock::now() - m_startTime);
    }
}
