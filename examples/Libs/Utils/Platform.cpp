#include "Platform.h"

namespace qe::Examples::Platform
{


void CpuRelax()
{
    #if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
            _mm_pause();
    #elif defined(__x86_64__) || defined(__i386__)
            __builtin_ia32_pause();
    #elif defined(__aarch64__) || defined(__arm64__)
            __asm__ __volatile__("yield");
    #else
        std::this_thread::yield();
    #endif
}

}
