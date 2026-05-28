#include "perfect_trace_backend.hpp"
#include "qe_trace_backend.hpp"
#include "trace_help.hpp"
#include "trace_script.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{

bool is_help_arg(const std::string& text)
{
    return (text == "--help") || (text == "-h") || (text == "help");
}

bool is_file_arg(const std::string& text)
{
    return (text == "--file") || (text == "-f");
}

bool read_script_file(const std::string& path, std::string& script_text, std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "could not open script file: " + path;
        return false;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    if (!input.good() && !input.eof())
    {
        error = "could not read script file: " + path;
        return false;
    }

    script_text = contents.str();
    return true;
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

    if ((argc != 3) && (argc != 4))
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
    std::string script_text;
    if (argc == 3)
    {
        script_text = argv[2];
    }
    else
    {
        if (!is_file_arg(argv[2]))
        {
            qe6502_trace::print_trace_help(std::cerr, program_name);
            return 1;
        }

        if (!read_script_file(argv[3], script_text, error))
        {
            std::cerr << "trace error: " << error << '\n';
            return 1;
        }
    }

    if (!qe6502_trace::parse_trace_script(script_text, script, error))
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
