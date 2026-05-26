#ifndef PERFECT6502_DEBUG_UI_HUMAN_HPP
#define PERFECT6502_DEBUG_UI_HUMAN_HPP

#include "debugger_core.hpp"

#include <cstdio>

namespace perfect6502_debug
{

void print_human_help(FILE* output);
void print_human_view(FILE* output, const DebuggerView& view);
void print_human_fullcycle_transition(FILE* output, const FullCycleTransitionView& transition);
void print_human_message(FILE* output, const char* message);
void print_human_error(FILE* output, const char* message);
void print_human_prompt(FILE* output);

} // namespace perfect6502_debug

#endif
