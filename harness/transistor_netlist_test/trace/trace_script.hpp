#ifndef QE6502_TRACE_SCRIPT_HPP
#define QE6502_TRACE_SCRIPT_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace qe6502_trace
{

enum class TraceBackend
{
    Perfect6502,
    Qe6502
};

struct MemoryWrite
{
    std::uint16_t address = 0u;
    std::vector<std::uint8_t> bytes;
};

enum class TraceCommandType
{
    Begin,
    Step,
    Irq,
    Nmi,
    Reset
};

struct TraceCommand
{
    TraceCommandType type = TraceCommandType::Begin;
    std::uint32_t count = 0u;
    std::uint8_t level = 1u;
};

struct TraceScript
{
    TraceBackend backend = TraceBackend::Perfect6502;
    std::vector<MemoryWrite> memory_writes;
    std::vector<TraceCommand> commands;
};

bool parse_trace_backend(const std::string& text, TraceBackend& backend);
bool parse_trace_script(const std::string& text, TraceScript& script, std::string& error);

const char* trace_backend_name(TraceBackend backend);
const char* trace_command_name(TraceCommandType type);

} // namespace qe6502_trace

#endif
