#include "cycle_cursor.hpp"

namespace perfect6502_debug
{

const char* cycle_boundary_name(CycleBoundary boundary)
{
    if (boundary == CycleBoundary::BeforeMemoryHalf)
    {
        return "BeforeMemoryHalf";
    }

    return "BeforeCpuHalf";
}

const char* next_halfcycle_name(CycleBoundary boundary)
{
    if (boundary == CycleBoundary::BeforeMemoryHalf)
    {
        return "M";
    }

    return "C";
}

bool is_full_cycle_boundary(const CycleCursor& cursor)
{
    return cursor.boundary == CycleBoundary::BeforeMemoryHalf;
}

void advance_one_halfcycle(CycleCursor& cursor)
{
    if (cursor.boundary == CycleBoundary::BeforeMemoryHalf)
    {
        cursor.boundary = CycleBoundary::BeforeCpuHalf;
    }
    else
    {
        cursor.boundary = CycleBoundary::BeforeMemoryHalf;
        ++cursor.cycle_number;
    }
}

} // namespace perfect6502_debug
