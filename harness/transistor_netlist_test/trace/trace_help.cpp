#include "trace_help.hpp"

#include <ostream>

namespace qe6502_trace
{

void print_trace_help(std::ostream& out, const char* program_name)
{
    out << "usage:\n"
        << "  " << program_name << " qe \"<script>\"\n"
        << "  " << program_name << " perfect \"<script>\"\n"
        << "  " << program_name << " qe -f <script-file>\n"
        << "  " << program_name << " perfect --file <script-file>\n"
        << "  " << program_name << " --help\n"
        << "\n"
        << "backends:\n"
        << "  qe, qe6502        run the qe6502_v2 backend\n"
        << "  perfect, perfect6502\n"
        << "                    run the perfect6502 transistor backend\n"
        << "\n"
        << "script input:\n"
        << "  Pass the script as one command-line string, or read it from a file with -f/--file.\n"
        << "\n"
        << "script statements:\n"
        << "  mem[addr]={bytes} load initial memory bytes\n"
        << "  begin             print the initial backend state\n"
        << "  s N               execute N full cycles; s100 is also accepted\n"
        << "  irq 0|1           set active-low IRQ input and print an event row\n"
        << "  nmi 0|1           set active-low NMI input and print an event row\n"
        << "  reset             restart/reset the backend and print an event row\n"
        << "\n"
        << "numbers:\n"
        << "  decimal, hex 0x..., binary b...\n"
        << "\n"
        << "notes:\n"
        << "  0 means asserted and 1 means deasserted for IRQ and NMI.\n"
        << "  Memory writes are initial setup and are applied before command execution.\n"
        << "  The trace starts with an info row that shows the first opcode fetch cycle and address.\n"
        << "  Step rows show CPU state after the full cycle and the next bus request. Read requests include the memory byte as bus=R AAAA={DD}.\n"
        << "  The qe backend marks regs=stable only when the tick is an opcode-fetch boundary suitable for register comparison.\n"
        << "  For perfect6502, each step executes memory halfcycle + CPU halfcycle and reads state after the CPU halfcycle.\n"
        << "\n"
        << "examples:\n"
        << "  " << program_name
        << " qe \"mem[0x200]={0x0e,0x0e,0x0e}; mem[0x2000]={0xab}; begin; s 3; nmi 0; s 1; nmi 1; s100;\"\n"
        << "  " << program_name << " qe -f script.trace\n";
}

} // namespace qe6502_trace
