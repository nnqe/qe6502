#include "perfect_trace_backend.hpp"
#include "qe_trace_backend.hpp"
#include "trace_help.hpp"
#include "trace_script.hpp"

#include <iostream>
#include <string>

namespace
{

bool is_help_arg(const std::string& text)
{
    return (text == "--help") || (text == "-h") || (text == "help");
}

} // namespace

int main(int argc, char** argv)
{
    const char* program_name = (argc > 0) ? argv[0] : "trace";

    if ((argc == 2) && is_help_arg(argv[1]))
    {
        qe6502_trace::print_trace_help(std::cout, program_name);
        return 0;
    }

    if (argc != 3)
    {
        qe6502_trace::print_trace_help(std::cerr, program_name);
        return 1;
    }

    qe6502_trace::TraceScript script;
    if (!qe6502_trace::parse_trace_backend(argv[1], script.backend))
    {
        std::cerr << "unknown backend: " << argv[1] << '\n';
        qe6502_trace::print_trace_help(std::cerr, program_name);
        return 1;
    }

    std::string error;
    if (!qe6502_trace::parse_trace_script(argv[2], script, error))
    {
        std::cerr << "trace parse error: " << error << '\n';
        return 1;
    }

    if (script.backend == qe6502_trace::TraceBackend::Qe6502)
    {
        if (!qe6502_trace::run_qe_trace(std::cout, script, error))
        {
            std::cerr << "trace error: " << error << '\n';
            return 1;
        }

        return 0;
    }

    if (!qe6502_trace::run_perfect_trace(std::cout, script, error))
    {
        std::cerr << "trace error: " << error << '\n';
        return 1;
    }

    return 0;
}
