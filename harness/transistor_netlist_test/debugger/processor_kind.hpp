#ifndef PERFECT6502_DEBUG_PROCESSOR_KIND_HPP
#define PERFECT6502_DEBUG_PROCESSOR_KIND_HPP

namespace perfect6502_debug
{

enum class ProcessorKind
{
    Perfect6502,
    Qe6502
};

const char* processor_kind_name(ProcessorKind processor);

} // namespace perfect6502_debug

#endif
