#include "command_loop.hpp"

#include <cstdio>
#include <cstring>

namespace
{

bool parse_processor(const char* text, perfect6502_debug::ProcessorKind& processor)
{
    if ((std::strcmp(text, "perfect6502") == 0) ||
        (std::strcmp(text, "perfect") == 0))
    {
        processor = perfect6502_debug::ProcessorKind::Perfect6502;
        return true;
    }

    if ((std::strcmp(text, "qe6502") == 0) ||
        (std::strcmp(text, "qe") == 0))
    {
        processor = perfect6502_debug::ProcessorKind::Qe6502;
        return true;
    }

    return false;
}

void print_usage(const char* program_name)
{
    (void)std::fprintf(stderr,
                       "usage: %s [perfect6502|qe6502]\n",
                       program_name);
}

} // namespace

int main(int argc, char** argv)
{
    perfect6502_debug::ProcessorKind processor = perfect6502_debug::ProcessorKind::Perfect6502;

    if (argc == 2)
    {
        if (!parse_processor(argv[1], processor))
        {
            print_usage((argc > 0) ? argv[0] : "perfect6502_debug");
            return 1;
        }
    }
    else if (argc != 1)
    {
        print_usage((argc > 0) ? argv[0] : "perfect6502_debug");
        return 1;
    }

    return perfect6502_debug::run_command_loop(processor);
}
