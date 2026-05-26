#ifndef PERFECT6502_DEBUG_CYCLE_CURSOR_HPP
#define PERFECT6502_DEBUG_CYCLE_CURSOR_HPP

#include <cstdint>

namespace perfect6502_debug
{

enum class CycleBoundary
{
    BeforeMemoryHalf,
    BeforeCpuHalf
};

struct CycleCursor
{
    std::uint64_t cycle_number = 0u;
    CycleBoundary boundary = CycleBoundary::BeforeMemoryHalf;
};

const char* cycle_boundary_name(CycleBoundary boundary);
const char* next_halfcycle_name(CycleBoundary boundary);
bool is_full_cycle_boundary(const CycleCursor& cursor);
void advance_one_halfcycle(CycleCursor& cursor);

} // namespace perfect6502_debug

#endif
