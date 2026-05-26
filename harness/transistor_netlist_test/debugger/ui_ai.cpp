#include "ui_ai.hpp"

#include "help_text.hpp"

#include <cstdio>

namespace perfect6502_debug
{
namespace
{

const char* short_interrupt_state_name(bool asserted)
{
    return asserted ? "asrt" : "rel";
}

void print_ai_memory_context(FILE* output, const BusStateView& bus)
{
    if (bus.memory_data_valid)
    {
        (void)std::fprintf(output, " mem=%02X", static_cast<unsigned>(bus.memory_data));
    }
}

void print_ai_cpu_input_bus(FILE* output, const BusStateView& bus)
{
    if (bus.read)
    {
        (void)std::fprintf(output,
                           "R %04X -> %02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(bus.data));
    }
    else
    {
        (void)std::fprintf(output,
                           "W %04X",
                           static_cast<unsigned>(bus.address));
    }

    print_ai_memory_context(output, bus);
}

std::uint8_t ai_cpu_output_write_data(const BusStateView& cpu_output_bus,
                                      const BusStateView& next_cpu_input_bus)
{
    if (!next_cpu_input_bus.read && (next_cpu_input_bus.address == cpu_output_bus.address))
    {
        return next_cpu_input_bus.data;
    }

    return cpu_output_bus.data;
}

void print_ai_cpu_output_bus(FILE* output,
                             const BusStateView& bus,
                             const BusStateView& next_cpu_input_bus)
{
    if (bus.read)
    {
        (void)std::fprintf(output,
                           "R %04X -> pending",
                           static_cast<unsigned>(bus.address));
    }
    else
    {
        const std::uint8_t write_data = ai_cpu_output_write_data(bus, next_cpu_input_bus);
        (void)std::fprintf(output,
                           "W %04X <= %02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(write_data));
    }

    print_ai_memory_context(output, bus);
}

void print_ai_input_data(FILE* output, const BusStateView& bus)
{
    if (bus.read)
    {
        (void)std::fprintf(output, "%02X", static_cast<unsigned>(bus.data));
    }
    else
    {
        (void)std::fputs("none", output);
    }
}

void print_ai_output_address_and_data(FILE* output,
                                      const BusStateView& bus,
                                      const BusStateView& next_cpu_input_bus)
{
    if (bus.read)
    {
        (void)std::fprintf(output,
                           "%04X|none",
                           static_cast<unsigned>(bus.address));
    }
    else
    {
        const std::uint8_t write_data = ai_cpu_output_write_data(bus, next_cpu_input_bus);
        (void)std::fprintf(output,
                           "%04X|%02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(write_data));
    }
}

void print_ai_registers(FILE* output, const char* prefix, const CpuRegisters& regs)
{
    (void)std::fprintf(output,
                       "%s pc=%04X ir=%02X a=%02X x=%02X y=%02X s=%02X p=%02X",
                       prefix,
                       static_cast<unsigned>(regs.pc),
                       static_cast<unsigned>(regs.ir),
                       static_cast<unsigned>(regs.a),
                       static_cast<unsigned>(regs.x),
                       static_cast<unsigned>(regs.y),
                       static_cast<unsigned>(regs.s),
                       static_cast<unsigned>(regs.p));
}

} // namespace

void print_ai_help(FILE* output)
{
    (void)std::fputs(ai_help_text(), output);
}

void print_ai_fullcycle_trace_line(FILE* output, const FullCycleTransitionView& transition)
{
    (void)std::fprintf(output,
                       "cy=%06llu | in ",
                       static_cast<unsigned long long>(transition.cycle_number));
    print_ai_cpu_input_bus(output, transition.cpu_input.bus);

    (void)std::fputs(" | input=", output);
    print_ai_input_data(output, transition.cpu_input.bus);

    (void)std::fputs(" | ", output);
    print_ai_registers(output, "pre", transition.cpu_input.registers);

    (void)std::fputs(" | out ", output);
    print_ai_cpu_output_bus(output, transition.cpu_output.bus, transition.next_cpu_input.bus);

    (void)std::fputs(" | output=", output);
    print_ai_output_address_and_data(output, transition.cpu_output.bus, transition.next_cpu_input.bus);

    (void)std::fputs(" | ", output);
    print_ai_registers(output, "post", transition.cpu_output.registers);

    (void)std::fprintf(output,
                       " | irq=%s nmi=%s\n",
                       short_interrupt_state_name(transition.next_cpu_input.interrupt_inputs.irq_asserted),
                       short_interrupt_state_name(transition.next_cpu_input.interrupt_inputs.nmi_asserted));
}

} // namespace perfect6502_debug
