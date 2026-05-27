#include "processor_kind.hpp"

namespace perfect6502_debug
{

const char* processor_kind_name(ProcessorKind processor)
{
    if (processor == ProcessorKind::Qe6502)
    {
        return "qe6502";
    }

    return "perfect6502";
}

} // namespace perfect6502_debug
