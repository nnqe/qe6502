#include "perfect_trace_backend.hpp"

extern "C"
{
#include "../third_party/perfect6502/perfect6502.h"
#include "../third_party/perfect6502/types.h"
#include "../third_party/perfect6502/netlist_sim.h"
}

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <string>

namespace qe6502_trace
{
namespace
{
constexpr std::size_t MemorySize = 65536u;
constexpr std::uint16_t StackPageBase = 0x0100u;
constexpr nodenum_t IrqNode = 103;
constexpr nodenum_t NmiNode = 1297;

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

void print_cycle_number(std::ostream& out, std::uint64_t cycle_number)
{
    out << std::dec << std::setw(6) << std::setfill('0') << cycle_number << std::setfill(' ');
}

std::uint16_t reset_vector_address(const std::uint8_t* memory_data)
{
    const std::uint16_t low = memory_data[0xfffcu];
    const std::uint16_t high = memory_data[0xfffdu];
    return static_cast<std::uint16_t>(low | static_cast<std::uint16_t>(high << 8u));
}


std::uint16_t stack_window_base(std::uint8_t s)
{
    const std::uint8_t first = static_cast<std::uint8_t>(s - 2u);
    return static_cast<std::uint16_t>(StackPageBase | first);
}

std::uint16_t stack_window_address(std::uint16_t base, std::uint16_t offset)
{
    const std::uint16_t low = static_cast<std::uint16_t>((base + offset) & 0x00ffu);
    return static_cast<std::uint16_t>(StackPageBase | low);
}

const char* active_low_text(std::uint8_t level)
{
    return (level == 0u) ? "asserted" : "deasserted";
}

bool active_low_asserted(std::uint8_t level)
{
    return level == 0u;
}

class PerfectTraceMachine
{
public:
    ~PerfectTraceMachine()
    {
        destroy();
    }

    PerfectTraceMachine(const PerfectTraceMachine&) = delete;
    PerfectTraceMachine& operator=(const PerfectTraceMachine&) = delete;

    PerfectTraceMachine() = default;

    bool initialize(std::string& error)
    {
        destroy();
        std::fill(memory, memory + MemorySize, 0u);
        cpu_ = initAndResetChip();
        if (cpu_ == nullptr)
        {
            error = "initAndResetChip failed";
            return false;
        }

        cycle_ = 0u;
        return true;
    }

    bool apply_memory_writes(const std::vector<MemoryWrite>& writes, std::string& error)
    {
        for (const MemoryWrite& write : writes)
        {
            const std::uint32_t address = write.address;
            const std::uint32_t bytes_available = static_cast<std::uint32_t>(MemorySize) - address;
            if (write.bytes.size() > bytes_available)
            {
                error = "memory write crosses the end of the 64K address space";
                return false;
            }

            for (std::size_t i = 0u; i < write.bytes.size(); ++i)
            {
                memory[static_cast<std::size_t>(address + i)] = write.bytes[i];
            }
        }

        return true;
    }

    void step_full_cycle()
    {
        step(cpu_);
        step(cpu_);
        cycle_++;
    }

    void set_irq(std::uint8_t level)
    {
        set_active_low_node(IrqNode, active_low_asserted(level));
    }

    void set_nmi(std::uint8_t level)
    {
        set_active_low_node(NmiNode, active_low_asserted(level));
    }

    bool reset(std::string& error)
    {
        destroy();
        cpu_ = initAndResetChip();
        if (cpu_ == nullptr)
        {
            error = "initAndResetChip failed";
            return false;
        }

        cycle_ = 0u;
        return true;
    }

    void print_boot_info(std::ostream& out) const
    {
        out << "info first_fetch_cycle=";
        print_cycle_number(out, 9u);
        out << " boot_cycles_before_first_fetch=8 first_fetch=";
        print_hex_word(out, reset_vector_address(memory));
        out << '\n';
    }

    void print_cycle_state(std::ostream& out) const
    {
        out << "cy=";
        print_cycle_number(out, cycle_);
        print_cpu_and_bus(out);
        out << '\n';
    }

    void print_event_state(std::ostream& out, const char* name) const
    {
        out << "ev=" << name << " cy=";
        print_cycle_number(out, cycle_);
        print_cpu_and_bus(out);
        out << '\n';
    }

    void print_interrupt_event(std::ostream& out, const char* name, std::uint8_t level) const
    {
        out << "ev=" << name << " level=" << static_cast<unsigned int>(level)
            << " state=" << active_low_text(level) << " cy=";
        print_cycle_number(out, cycle_);
        print_cpu_and_bus(out);
        out << '\n';
    }

private:
    void destroy()
    {
        if (cpu_ != nullptr)
        {
            destroyChip(cpu_);
            cpu_ = nullptr;
        }
    }

    void set_active_low_node(nodenum_t node, bool asserted)
    {
        setNode(static_cast<state_t*>(cpu_), node, asserted ? 0 : 1);
        recalcNodeList(static_cast<state_t*>(cpu_));
    }

    void print_cpu_and_bus(std::ostream& out) const
    {
        out << " pc=";
        print_hex_word(out, readPC(cpu_));
        out << " a=";
        print_hex_byte(out, readA(cpu_));
        out << " x=";
        print_hex_byte(out, readX(cpu_));
        out << " y=";
        print_hex_byte(out, readY(cpu_));
        out << " s=";
        print_hex_byte(out, readSP(cpu_));
        out << " p=";
        print_hex_byte(out, readP(cpu_));
        out << ' ';
        print_stack(out);
        out << " bus=";
        print_bus(out);
    }

    void print_stack(std::ostream& out) const
    {
        const std::uint16_t base = stack_window_base(readSP(cpu_));
        out << "mem[";
        print_hex_word(out, base);
        out << "]={";

        for (std::uint16_t i = 0u; i < 5u; ++i)
        {
            if (i != 0u)
            {
                out << ',';
            }
            print_hex_byte(out, memory[stack_window_address(base, i)]);
        }

        out << '}';
    }

    void print_bus(std::ostream& out) const
    {
        if (readRW(cpu_) == 0u)
        {
            out << "W ";
            print_hex_word(out, readAddressBus(cpu_));
            out << '=';
            print_hex_byte(out, readDataBus(cpu_));
        }
        else
        {
            out << "R ";
            const std::uint16_t address = readAddressBus(cpu_);
            print_hex_word(out, address);
            out << "={";
            print_hex_byte(out, memory[address]);
            out << "}";
        }
    }

    state_t* cpu_ = nullptr;
    std::uint64_t cycle_ = 0u;
};

} // namespace

bool run_perfect_trace(std::ostream& out, const TraceScript& script, std::string& error)
{
    PerfectTraceMachine machine;
    if (!machine.initialize(error))
    {
        return false;
    }

    if (!machine.apply_memory_writes(script.memory_writes, error))
    {
        return false;
    }

    machine.print_boot_info(out);

    for (const TraceCommand& command : script.commands)
    {
        switch (command.type)
        {
            case TraceCommandType::Begin:
                machine.print_event_state(out, "begin");
                break;

            case TraceCommandType::Step:
                for (std::uint32_t i = 0u; i < command.count; ++i)
                {
                    machine.step_full_cycle();
                    machine.print_cycle_state(out);
                }
                break;

            case TraceCommandType::Irq:
                machine.set_irq(command.level);
                machine.print_interrupt_event(out, "irq", command.level);
                break;

            case TraceCommandType::Nmi:
                machine.set_nmi(command.level);
                machine.print_interrupt_event(out, "nmi", command.level);
                break;

            case TraceCommandType::Reset:
                if (!machine.reset(error))
                {
                    return false;
                }
                machine.print_event_state(out, "reset");
                break;
        }
    }

    return true;
}

} // namespace qe6502_trace
