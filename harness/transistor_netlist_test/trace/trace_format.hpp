#ifndef QE6502_TRACE_FORMAT_HPP
#define QE6502_TRACE_FORMAT_HPP

#include "trace_script.hpp"

#include <iosfwd>

namespace qe6502_trace
{

void print_parsed_script(std::ostream& out, const TraceScript& script);

} // namespace qe6502_trace

#endif
