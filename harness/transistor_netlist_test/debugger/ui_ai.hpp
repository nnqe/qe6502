#ifndef PERFECT6502_DEBUG_UI_AI_HPP
#define PERFECT6502_DEBUG_UI_AI_HPP

#include "debugger_core.hpp"

#include <cstdio>

namespace perfect6502_debug
{

void print_ai_help(FILE* output);
void print_ai_fullcycle_trace_line(FILE* output, const FullCycleTransitionView& transition);

} // namespace perfect6502_debug

#endif
