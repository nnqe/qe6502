#include "ui_human.hpp"

#include "cycle_cursor.hpp"
#include "help_text.hpp"

#include <cstdio>

namespace perfect6502_debug
{
namespace
{

void print_flag(FILE* output, const char* name, bool value)
{
    (void)std::fprintf(output, "%s=%u ", name, value ? 1u : 0u);
}

void print_flags(FILE* output, std::uint8_t p)
{
    print_flag(output, "N", (p & 0x80u) != 0u);
    print_flag(output, "V", (p & 0x40u) != 0u);
    print_flag(output, "U", (p & 0x20u) != 0u);
    print_flag(output, "B", (p & 0x10u) != 0u);
    print_flag(output, "D", (p & 0x08u) != 0u);
    print_flag(output, "I", (p & 0x04u) != 0u);
    print_flag(output, "Z", (p & 0x02u) != 0u);
    print_flag(output, "C", (p & 0x01u) != 0u);
}

const char* input_state_name(bool asserted)
{
    return asserted ? "asserted" : "released";
}

const char* cycle_mode_name(DebuggerCycleMode mode)
{
    if (mode == DebuggerCycleMode::Fullcycle)
    {
        return "fullcycle-on";
    }

    return "fullcycle-off";
}

void print_bus(FILE* output, const BusStateView& bus)
{
    if (bus.served)
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
                               "W %04X <= %02X",
                               static_cast<unsigned>(bus.address),
                               static_cast<unsigned>(bus.data));
        }
    }
    else
    {
        if (bus.read)
        {
            if (bus.memory_data_valid)
            {
                (void)std::fprintf(output,
                                   "R %04X -> ?? (bus=%02X, mem=%02X)",
                                   static_cast<unsigned>(bus.address),
                                   static_cast<unsigned>(bus.data),
                                   static_cast<unsigned>(bus.memory_data));
            }
            else
            {
                (void)std::fprintf(output,
                                   "R %04X -> ?? (bus=%02X)",
                                   static_cast<unsigned>(bus.address),
                                   static_cast<unsigned>(bus.data));
            }
        }
        else
        {
            (void)std::fprintf(output,
                               "W %04X <= ?? (bus=%02X)",
                               static_cast<unsigned>(bus.address),
                               static_cast<unsigned>(bus.data));
        }
    }
}

void print_registers_inline(FILE* output, const CpuRegisters& regs)
{
    (void)std::fprintf(output,
                       "PC=%04X IR=%02X A=%02X X=%02X Y=%02X S=%02X P=%02X",
                       static_cast<unsigned>(regs.pc),
                       static_cast<unsigned>(regs.ir),
                       static_cast<unsigned>(regs.a),
                       static_cast<unsigned>(regs.x),
                       static_cast<unsigned>(regs.y),
                       static_cast<unsigned>(regs.s),
                       static_cast<unsigned>(regs.p));
}

void print_memory_context(FILE* output, const BusStateView& bus)
{
    if (bus.memory_data_valid)
    {
        (void)std::fprintf(output,
                           "mem[%04X]=%02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(bus.memory_data));
    }
    else
    {
        (void)std::fprintf(output,
                           "mem[%04X]=??",
                           static_cast<unsigned>(bus.address));
    }
}

void print_cpu_input_bus(FILE* output, const BusStateView& bus)
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
}

std::uint8_t cpu_output_write_data(const BusStateView& cpu_output_bus, const BusStateView& next_cpu_input_bus)
{
    if (!next_cpu_input_bus.read && (next_cpu_input_bus.address == cpu_output_bus.address))
    {
        return next_cpu_input_bus.data;
    }

    return cpu_output_bus.data;
}

void print_cpu_output_bus(FILE* output,
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
        const std::uint8_t write_data = cpu_output_write_data(bus, next_cpu_input_bus);
        (void)std::fprintf(output,
                           "W %04X <= %02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(write_data));
    }
}

void print_cpu_input_data(FILE* output, const BusStateView& bus)
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

void print_cpu_output_address_and_data(FILE* output,
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
        const std::uint8_t write_data = cpu_output_write_data(bus, next_cpu_input_bus);
        (void)std::fprintf(output,
                           "%04X|%02X",
                           static_cast<unsigned>(bus.address),
                           static_cast<unsigned>(write_data));
    }
}

void print_cpu_input_point(FILE* output, const char* title, const CpuPointView& point)
{
    (void)std::fprintf(output, "%s\n", title);

    (void)std::fputs("  bus:        ", output);
    print_cpu_input_bus(output, point.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  mem:        ", output);
    print_memory_context(output, point.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  input data: ", output);
    print_cpu_input_data(output, point.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  regs:       ", output);
    print_registers_inline(output, point.registers);
    (void)std::fputc('\n', output);

    (void)std::fprintf(output,
                       "  inputs:     irq=%s nmi=%s\n",
                       input_state_name(point.interrupt_inputs.irq_asserted),
                       input_state_name(point.interrupt_inputs.nmi_asserted));
}

void print_cpu_output_point(FILE* output,
                            const char* title,
                            const CpuPointView& point,
                            const CpuPointView& next_cpu_input)
{
    (void)std::fprintf(output, "%s\n", title);

    (void)std::fputs("  bus:              ", output);
    print_cpu_output_bus(output, point.bus, next_cpu_input.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  mem:              ", output);
    print_memory_context(output, point.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  output addr|data: ", output);
    print_cpu_output_address_and_data(output, point.bus, next_cpu_input.bus);
    (void)std::fputc('\n', output);

    (void)std::fputs("  regs:             ", output);
    print_registers_inline(output, point.registers);
    (void)std::fputc('\n', output);

    (void)std::fprintf(output,
                       "  inputs:           irq=%s nmi=%s\n",
                       input_state_name(point.interrupt_inputs.irq_asserted),
                       input_state_name(point.interrupt_inputs.nmi_asserted));
}

void print_memory_bytes(FILE* output, const char* title, const std::vector<MemoryByteView>& bytes)
{
    (void)std::fprintf(output, "%s\n", title);
    for (const MemoryByteView& byte : bytes)
    {
        (void)std::fprintf(output,
                           "  %04X: %02X%s\n",
                           static_cast<unsigned>(byte.address),
                           static_cast<unsigned>(byte.value),
                           byte.marker ? "  <--" : "");
    }
}

} // namespace

void print_human_help(FILE* output)
{
    (void)std::fputs(human_help_text(), output);
}

void print_human_view(FILE* output, const DebuggerView& view)
{
    const CpuPointView& point = view.point;
    const CpuRegisters& regs = point.registers;

    (void)std::fprintf(output,
                       "mode: %s\n",
                       cycle_mode_name(view.cycle_mode));

    (void)std::fprintf(output,
                       "cursor: cycle=%llu boundary=%s next_half=%s perfect_half=%llu\n",
                       static_cast<unsigned long long>(point.cursor.cycle_number),
                       cycle_boundary_name(point.cursor.boundary),
                       next_halfcycle_name(point.cursor.boundary),
                       static_cast<unsigned long long>(point.perfect_half_cycle));

    (void)std::fputs("bus: ", output);
    print_bus(output, point.bus);
    (void)std::fputc('\n', output);

    (void)std::fprintf(output,
                       "regs: PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X IR=%02X\n",
                       static_cast<unsigned>(regs.pc),
                       static_cast<unsigned>(regs.a),
                       static_cast<unsigned>(regs.x),
                       static_cast<unsigned>(regs.y),
                       static_cast<unsigned>(regs.s),
                       static_cast<unsigned>(regs.p),
                       static_cast<unsigned>(regs.ir));

    (void)std::fputs("flags: ", output);
    print_flags(output, regs.p);
    (void)std::fputc('\n', output);

    (void)std::fprintf(output,
                       "inputs: irq=%s nmi=%s\n",
                       input_state_name(point.interrupt_inputs.irq_asserted),
                       input_state_name(point.interrupt_inputs.nmi_asserted));

    print_memory_bytes(output, "stack around S:", view.stack_bytes);
    print_memory_bytes(output, "bytes at PC:", view.pc_bytes);
}

void print_human_fullcycle_transition(FILE* output, const FullCycleTransitionView& transition)
{
    (void)std::fprintf(output,
                       "cycle %llu completed\n",
                       static_cast<unsigned long long>(transition.cycle_number));

    print_cpu_input_point(output, "cpu-in:", transition.cpu_input);
    print_cpu_output_point(output, "cpu-out:", transition.cpu_output, transition.next_cpu_input);
    print_cpu_input_point(output, "next cpu-in:", transition.next_cpu_input);
}

void print_human_message(FILE* output, const char* message)
{
    (void)std::fprintf(output, "%s\n", message);
}

void print_human_error(FILE* output, const char* message)
{
    (void)std::fprintf(output, "error: %s\n", message);
}

void print_human_prompt(FILE* output)
{
    (void)std::fputs("> ", output);
    (void)std::fflush(output);
}

} // namespace perfect6502_debug
