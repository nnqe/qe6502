#ifndef PERFECT6502_DEBUG_COMMAND_INPUT_HPP
#define PERFECT6502_DEBUG_COMMAND_INPUT_HPP

#include <cstdint>
#include <string>

namespace perfect6502_debug
{

enum class CommandKind
{
    Empty,
    Help,
    Half,
    Cycle,
    Undo,
    Irq,
    Nmi,
    Fullcycle,
    Show,
    Setup,
    Quit
};

struct DebugCommand
{
    CommandKind kind = CommandKind::Empty;
    std::uint32_t count = 1u;
    std::string setup_text;
    std::string help_topic;
    bool asserted = false;
    bool toggle = false;
    bool fullcycle_enabled = false;
    bool repeatable = false;
};

bool parse_debug_command(const std::string& line, DebugCommand& command, std::string& error);

} // namespace perfect6502_debug

#endif
