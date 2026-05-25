#include "temporary_tests.hpp"

int main()
{
    if (!perfect6502_debug::test_bootstrap())
    {
        return 1;
    }

    if (!perfect6502_debug::test_snapshot_restore())
    {
        return 1;
    }

    return 0;
}
