#include "Clock.h"

#include "SpinLock.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <mutex>

namespace qe::Examples
{
    using std::chrono::duration_cast;
    using std::chrono::steady_clock;

    namespace
    {
        SpinLock lock_;
        steady_clock::time_point pivot_ = steady_clock::now();
        Clock::duration offset_ = {};
        double speed_ = 1.0;
    }

    Clock::time_point Clock::now() noexcept
    {
        std::lock_guard guard(lock_);
        auto now = steady_clock::now();
        auto delta = duration_cast<duration>(now - pivot_);
        return time_point(offset_ + duration(std::llround(double(delta.count()) * speed_)));
    }

    double Clock::GetSpeed() noexcept
    {
        std::lock_guard guard(lock_);
        return speed_;
    }

    void Clock::SetSpeed(double speed) noexcept
    {
        if constexpr (is_steady)
        {
            assert(speed >= 0);
            speed = std::max(0.0, speed);
        }
        std::lock_guard guard(lock_);
        auto now = steady_clock::now();
        auto delta = duration_cast<Clock::duration>(now - pivot_);
        pivot_ = now;
        offset_ += duration(std::llround(double(delta.count()) * speed_));
        speed_ = speed;
    }
}
