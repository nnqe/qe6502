#pragma once
#include <atomic>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace qe::Examples
{
    /// @brief Gives hint to processor that improves performance of spin-wait loops.
    static inline void SpinWaitHint()
    {
        // Generates the PAUSE x86 instruction if available
#if defined(_MSC_VER)
        _mm_pause();
#elif defined(__GNUC__)
        __builtin_ia32_pause();
#else
        std::this_thread::yield();
#endif
    }

    /// @brief TTAS (test and test-and-set) spin lock with PAUSE instruction hint
    class SpinLock
    {
    public:
        inline bool try_lock() noexcept
        {
            if (m_locked.load(std::memory_order::relaxed))  // test
            {
                return false;
            }
            if (m_locked.exchange(true, std::memory_order_acquire))  // test-and-set
            {
                return false;
            }
            return true;
        }

        inline void lock() noexcept
        {
            for (;;)
            {
                // Note: avoid spinning on a read-modify-write operation to reduce cache misses.
                if (!m_locked.exchange(true, std::memory_order_acquire))  // test-and-set
                {
                    return;
                }

                // Wait for lock to be released before attempting to lock again.
                while (m_locked.load(std::memory_order::relaxed))  // test
                {
                    SpinWaitHint();
                }
            }
        }

        inline void unlock() noexcept
        {
            m_locked.store(false, std::memory_order_release);
        }

    private:
        std::atomic_bool m_locked{false};
    };
}
