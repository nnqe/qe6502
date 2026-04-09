#pragma once
#include <chrono>

namespace qe::Examples
{
    class Clock
    {
    public:
        using duration = std::chrono::steady_clock::duration;
        using rep = duration::rep;
        using period = duration::period;
        using time_point = std::chrono::time_point<Clock>;
        using Seconds = std::chrono::seconds;
        using Millis = std::chrono::milliseconds;
        using Micros = std::chrono::microseconds;
        using Nanos = std::chrono::nanoseconds;

        static constexpr bool is_steady = true;

        static time_point now() noexcept;

    public:
        static double ToSeconds(duration d);
        static double GetSpeed() noexcept;
        static void SetSpeed(double) noexcept;
    };

    // TODO: Fix for mac
    //static_assert(std::chrono::is_clock_v<Clock>);
}
