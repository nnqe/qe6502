#include "command_loop.hpp"

#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        return perfect6502_debug::run_command_loop();
    }

    (void)std::fprintf(stderr,
                       "usage: %s\n",
                       (argc > 0) ? argv[0] : "perfect6502_debug");
    return 1;
}
