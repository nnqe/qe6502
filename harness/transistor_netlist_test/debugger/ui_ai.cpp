#include "ui_ai.hpp"

#include "help_text.hpp"

#include <cstdio>

namespace perfect6502_debug
{

void print_ai_help(FILE* output)
{
    (void)std::fputs(ai_help_text(), output);
}

} // namespace perfect6502_debug
