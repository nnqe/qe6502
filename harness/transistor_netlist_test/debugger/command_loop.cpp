#include "command_loop.hpp"

#include "command_input.hpp"
#include "debugger_core.hpp"
#include "ui_ai.hpp"
#include "ui_human.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace perfect6502_debug
{
namespace
{

bool execute_command(DebuggerCore& debugger, const DebugCommand& command)
{
    std::string error;

    switch (command.kind)
    {
    case CommandKind::Empty:
        return true;

    case CommandKind::Help:
        if (command.help_topic.empty() || (command.help_topic == "human"))
        {
            print_human_help(stdout);
            return true;
        }

        if (command.help_topic == "ai")
        {
            print_ai_help(stdout);
            return true;
        }

        print_human_error(stderr, "unknown help topic; use help, help human, or help ai");
        return true;

    case CommandKind::Half:
        if (!debugger.step_halfcycles(command.count, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Cycle:
    {
        std::vector<FullCycleTransitionView> transitions;
        if (!debugger.step_full_cycles(command.count, transitions, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }

        if (!transitions.empty())
        {
            for (const FullCycleTransitionView& transition : transitions)
            {
                print_human_fullcycle_transition(stdout, transition);
            }
        }
        else
        {
            print_human_view(stdout, debugger.make_view());
        }

        return true;
    }

    case CommandKind::Trace:
    {
        if (debugger.cycle_mode() != DebuggerCycleMode::Fullcycle)
        {
            print_human_error(stderr, "trace requires fullcycle mode on");
            return true;
        }

        std::vector<FullCycleTransitionView> transitions;
        if (!debugger.step_full_cycles(command.count, transitions, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }

        /*
            Scripted runs do not echo the command line, so the prompt text remains
            on stdout without a trailing newline. Trace output is meant to be parsed
            mechanically, with each record starting at column zero. Emit one separator
            newline before the first trace record so every trace line starts with "cy=".
        */
        print_human_message(stdout, "");

        for (const FullCycleTransitionView& transition : transitions)
        {
            print_ai_fullcycle_trace_line(stdout, transition);
        }

        return true;
    }

    case CommandKind::Undo:
        if (!debugger.undo(command.count, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Irq:
        if (!debugger.set_irq_asserted(command.asserted, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Fullcycle:
        if (!debugger.set_fullcycle_mode(command.fullcycle_enabled, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Nmi:
        if (command.toggle)
        {
            if (!debugger.toggle_nmi(error))
            {
                print_human_error(stderr, error.c_str());
                return true;
            }
        }
        else if (!debugger.set_nmi_asserted(command.asserted, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Show:
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Setup:
        if (!debugger.apply_setup_text(command.setup_text, error))
        {
            print_human_error(stderr, error.c_str());
            return true;
        }
        print_human_view(stdout, debugger.make_view());
        return true;

    case CommandKind::Quit:
        return false;
    }

    print_human_error(stderr, "internal command dispatch error");
    return true;
}

} // namespace

int run_command_loop(ProcessorKind processor)
{
    DebuggerCore debugger(processor);
    std::string error;

    if (!debugger.initialize(error))
    {
        print_human_error(stderr, error.c_str());
        return 1;
    }

    print_human_message(stdout, "perfect6502_debug interactive loop");
    const std::string processor_message = std::string("processor: ") + processor_kind_name(processor);
    print_human_message(stdout, processor_message.c_str());
    print_human_message(stdout, "type help for human-oriented help");
    print_human_message(stdout, "type help ai for AI-oriented help");
    print_human_message(stdout, "type q to quit");
    print_human_view(stdout, debugger.make_view());

    DebugCommand last_repeatable_command;
    bool has_repeatable_command = false;

    while (true)
    {
        print_human_prompt(stdout);

        std::string line;
        if (!std::getline(std::cin, line))
        {
            print_human_message(stdout, "");
            break;
        }

        DebugCommand command;
        if (!parse_debug_command(line, command, error))
        {
            print_human_error(stderr, error.c_str());
            continue;
        }

        if (command.kind == CommandKind::Empty)
        {
            if (!has_repeatable_command)
            {
                continue;
            }

            command = last_repeatable_command;
        }
        else if (command.repeatable)
        {
            last_repeatable_command = command;
            has_repeatable_command = true;
        }

        if (!execute_command(debugger, command))
        {
            break;
        }
    }

    return 0;
}

} // namespace perfect6502_debug
