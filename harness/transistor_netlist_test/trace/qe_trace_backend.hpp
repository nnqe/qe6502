#ifndef QE6502_QE_TRACE_BACKEND_HPP
#define QE6502_QE_TRACE_BACKEND_HPP

#include "trace_script.hpp"

#include <iosfwd>
#include <string>

namespace qe6502_trace
{

bool run_qe_trace(std::ostream& out, const TraceScript& script, std::string& error);

} // namespace qe6502_trace

#endif
