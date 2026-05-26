#include "command_loop.hpp"

#include <cstdio>
#include <string>

namespace perfect6502_debug
{

int run_illegal_opcode_investigator(const char* executable_path);

} // namespace perfect6502_debug

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        return perfect6502_debug::run_command_loop();
    }

    if ((argc == 2) && (std::string(argv[1]) == "investigate-illegal"))
    {
        return perfect6502_debug::run_illegal_opcode_investigator(argv[0]);
    }

    (void)std::fprintf(stderr,
                       "usage: %s [investigate-illegal]\n",
                       (argc > 0) ? argv[0] : "perfect6502_debug");
    return 1;
}
