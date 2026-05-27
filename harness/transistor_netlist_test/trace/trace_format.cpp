#include "trace_format.hpp"

#include <cstdint>
#include <iomanip>
#include <ostream>

namespace qe6502_trace
{
namespace
{
void print_hex_byte(std::ostream& out, std::uint8_t value)
{
    out << std::hex << std::nouppercase << std::setw(2) << std::setfill('0')
        << static_cast<unsigned int>(value) << std::dec << std::setfill(' ');
}

void print_hex_word(std::ostream& out, std::uint16_t value)
{
    out << std::hex << std::nouppercase << std::setw(4) << std::setfill('0')
        << static_cast<unsigned int>(value) << std::dec << std::setfill(' ');
}

} // namespace

void print_parsed_script(std::ostream& out, const TraceScript& script)
{
    out << "backend=" << trace_backend_name(script.backend) << '\n';

    for (const MemoryWrite& write : script.memory_writes)
    {
        out << "mem[";
        print_hex_word(out, write.address);
        out << "]={";

        for (std::size_t i = 0u; i < write.bytes.size(); ++i)
        {
            if (i != 0u)
            {
                out << ',';
            }
            print_hex_byte(out, write.bytes[i]);
        }

        out << "}" << '\n';
    }

    for (const TraceCommand& command : script.commands)
    {
        out << "cmd=" << trace_command_name(command.type);

        if (command.type == TraceCommandType::Step)
        {
            out << " count=" << command.count;
        }
        else if ((command.type == TraceCommandType::Irq) ||
                 (command.type == TraceCommandType::Nmi))
        {
            out << " level=" << static_cast<unsigned int>(command.level);
        }

        out << '\n';
    }
}

} // namespace qe6502_trace
