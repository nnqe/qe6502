#include "command_input.hpp"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace perfect6502_debug
{
namespace
{

char lower_ascii(char ch)
{
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return static_cast<char>(ch - 'A' + 'a');
    }

    return ch;
}

std::string lowercase_copy(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    for (char ch : text)
    {
        result.push_back(lower_ascii(ch));
    }

    return result;
}

std::string trim_copy(const std::string& text)
{
    std::size_t begin = 0u;
    while ((begin < text.size()) &&
           ((text[begin] == ' ') || (text[begin] == '\t') ||
            (text[begin] == '\r') || (text[begin] == '\n')))
    {
        ++begin;
    }

    std::size_t end = text.size();
    while ((end > begin) &&
           ((text[end - 1u] == ' ') || (text[end - 1u] == '\t') ||
            (text[end - 1u] == '\r') || (text[end - 1u] == '\n')))
    {
        --end;
    }

    return text.substr(begin, end - begin);
}

bool parse_count(std::istringstream& input, std::uint32_t& count, std::string& error)
{
    std::string token;
    if (!(input >> token))
    {
        count = 1u;
        return true;
    }

    std::uint64_t value = 0u;
    for (char ch : token)
    {
        if ((ch < '0') || (ch > '9'))
        {
            error = "command count must be a positive decimal integer";
            return false;
        }

        value = (value * 10u) + static_cast<std::uint64_t>(ch - '0');
        if (value > 0xffffffffu)
        {
            error = "command count is too large";
            return false;
        }
    }

    if (value == 0u)
    {
        error = "command count must be positive";
        return false;
    }

    std::string extra;
    if (input >> extra)
    {
        error = "unexpected extra command argument";
        return false;
    }

    count = static_cast<std::uint32_t>(value);
    return true;
}

std::string rest_after_first_word(const std::string& line)
{
    std::size_t pos = 0u;
    while ((pos < line.size()) &&
           (line[pos] != ' ') && (line[pos] != '\t') &&
           (line[pos] != '\r') && (line[pos] != '\n'))
    {
        ++pos;
    }

    return trim_copy(line.substr(pos));
}

bool parse_interrupt_action(std::istringstream& input,
                            bool allow_toggle,
                            bool& asserted,
                            bool& toggle,
                            std::string& error)
{
    std::string action;
    if (!(input >> action))
    {
        error = "interrupt command requires an action";
        return false;
    }

    action = lowercase_copy(action);

    if ((action == "assert") || (action == "a"))
    {
        asserted = true;
        toggle = false;
    }
    else if ((action == "release") || (action == "r"))
    {
        asserted = false;
        toggle = false;
    }
    else if (allow_toggle && ((action == "toggle") || (action == "t")))
    {
        asserted = false;
        toggle = true;
    }
    else
    {
        error = allow_toggle
            ? "interrupt action must be assert, release, toggle, a, r, or t"
            : "interrupt action must be assert, release, a, or r";
        return false;
    }

    std::string extra;
    if (input >> extra)
    {
        error = "unexpected extra interrupt command argument";
        return false;
    }

    return true;
}

} // namespace

bool parse_debug_command(const std::string& line, DebugCommand& command, std::string& error)
{
    command = DebugCommand();

    const std::string trimmed = trim_copy(line);
    if (trimmed.empty())
    {
        command.kind = CommandKind::Empty;
        return true;
    }

    std::istringstream input(trimmed);
    std::string word;
    input >> word;
    word = lowercase_copy(word);

    if ((word == "q") || (word == "quit"))
    {
        command.kind = CommandKind::Quit;
        return true;
    }

    if (word == "help")
    {
        command.kind = CommandKind::Help;
        std::string topic;
        if (input >> topic)
        {
            command.help_topic = lowercase_copy(topic);
        }

        std::string extra;
        if (input >> extra)
        {
            error = "help accepts at most one topic";
            return false;
        }

        return true;
    }

    if ((word == "h") || (word == "half"))
    {
        command.kind = CommandKind::Half;
        command.repeatable = true;
        return parse_count(input, command.count, error);
    }

    if ((word == "c") || (word == "cycle"))
    {
        command.kind = CommandKind::Cycle;
        command.repeatable = true;
        return parse_count(input, command.count, error);
    }

    if ((word == "t") || (word == "trace"))
    {
        command.kind = CommandKind::Trace;
        command.repeatable = true;
        return parse_count(input, command.count, error);
    }

    if ((word == "u") || (word == "undo"))
    {
        command.kind = CommandKind::Undo;
        command.repeatable = true;
        return parse_count(input, command.count, error);
    }


    if ((word == "fullcycle") || (word == "fc"))
    {
        command.kind = CommandKind::Fullcycle;

        std::string action;
        if (!(input >> action))
        {
            error = "fullcycle requires on or off";
            return false;
        }

        action = lowercase_copy(action);
        if (action == "on")
        {
            command.fullcycle_enabled = true;
        }
        else if (action == "off")
        {
            command.fullcycle_enabled = false;
        }
        else
        {
            error = "fullcycle action must be on or off";
            return false;
        }

        std::string extra;
        if (input >> extra)
        {
            error = "unexpected extra fullcycle command argument";
            return false;
        }

        return true;
    }

    if (word == "irq")
    {
        command.kind = CommandKind::Irq;
        return parse_interrupt_action(input, false, command.asserted, command.toggle, error);
    }

    if (word == "nmi")
    {
        command.kind = CommandKind::Nmi;
        return parse_interrupt_action(input, true, command.asserted, command.toggle, error);
    }

    if (word == "show")
    {
        command.kind = CommandKind::Show;
        std::string extra;
        if (input >> extra)
        {
            error = "show does not accept arguments";
            return false;
        }

        return true;
    }

    if (word == "setup")
    {
        command.kind = CommandKind::Setup;
        command.setup_text = rest_after_first_word(trimmed);
        if (command.setup_text.empty())
        {
            error = "setup requires a setup expression";
            return false;
        }

        return true;
    }

    error = "unknown command '" + word + "'";
    return false;
}

} // namespace perfect6502_debug
