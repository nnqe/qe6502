#include "qe_trace_backend.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <string>

extern "C"
{
#include "qe6502.h"
}

namespace qe6502_trace
{
namespace
{
constexpr std::size_t MemorySize = 65536u;
constexpr std::uint16_t StackPageBase = 0x0100u;

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

void print_cycle_number(std::ostream& out, std::uint64_t cycle)
{
    out << std::dec << std::setw(6) << std::setfill('0') << cycle << std::setfill(' ');
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

const char* register_compare_state(qe6502_tick_t tick)
{
    return ((tick.status & qe6502_status_opcode_fetch) != 0u) ? "stable" : "unstable";
}

class QeTraceMachine
{
public:
    QeTraceMachine()
    {
        cpu_.model = qe6502_model_nmos;
        tick_ = qe6502_restart(&cpu_);
    }

    bool apply_memory_writes(const std::vector<MemoryWrite>& writes, std::string& error)
    {
        for (const MemoryWrite& write : writes)
        {
            const std::uint32_t address = write.address;
            const std::uint32_t bytes_available = static_cast<std::uint32_t>(memory_.size()) - address;
            if (write.bytes.size() > bytes_available)
            {
                error = "memory write crosses the end of the 64K address space";
                return false;
            }

            for (std::size_t i = 0u; i < write.bytes.size(); ++i)
            {
                memory_[static_cast<std::size_t>(address + i)] = write.bytes[i];
            }
        }

        return true;
    }

    void step()
    {
        process_memory();
        tick_ = qe6502_tick(&cpu_, tick_.bus);
        cycle_++;
    }

    void set_irq(std::uint8_t level)
    {
        qe6502_set_irq(&cpu_, level);
    }

    void set_nmi(std::uint8_t level)
    {
        if (0 == level){
            qe6502_nmi(&cpu_);
        }
    }

    void reset()
    {
        tick_ = qe6502_reset(&cpu_);
    }

    void print_boot_info(std::ostream& out) const
    {
        out << "info first_fetch_cycle=";
        print_cycle_number(out, 9u);
        out << " boot_cycles_before_first_fetch=8 first_fetch=";
        print_hex_word(out, reset_vector_address(memory_.data()));
        out << '\n';
    }

    void print_cycle_state(std::ostream& out) const
    {
        out << "cy=";
        print_cycle_number(out, cycle_);
        out << " pc=";
        print_hex_word(out, cpu_.PC);
        out << " a=";
        print_hex_byte(out, cpu_.A);
        out << " x=";
        print_hex_byte(out, cpu_.X);
        out << " y=";
        print_hex_byte(out, cpu_.Y);
        out << " s=";
        print_hex_byte(out, cpu_.S);
        out << " p=";
        print_hex_byte(out, cpu_.P);
        out << " regs=" << register_compare_state(tick_);
        out << ' ';
        print_stack(out);
        out << " bus=";
        print_bus(out);
        out << '\n';
    }


    void print_event_state(std::ostream& out, const char* name) const
    {
        out << "ev=" << name << " cy=";
        print_cycle_number(out, cycle_);
        out << " pc=";
        print_hex_word(out, cpu_.PC);
        out << " a=";
        print_hex_byte(out, cpu_.A);
        out << " x=";
        print_hex_byte(out, cpu_.X);
        out << " y=";
        print_hex_byte(out, cpu_.Y);
        out << " s=";
        print_hex_byte(out, cpu_.S);
        out << " p=";
        print_hex_byte(out, cpu_.P);
        out << " regs=" << register_compare_state(tick_);
        out << ' ';
        print_stack(out);
        out << " bus=";
        print_bus(out);
        out << '\n';
    }

    void print_interrupt_event(std::ostream& out, const char* name, std::uint8_t level) const
    {
        out << "ev=" << name << " level=" << static_cast<unsigned int>(level)
            << " state=" << active_low_text(level) << " cy=";
        print_cycle_number(out, cycle_);
        out << " pc=";
        print_hex_word(out, cpu_.PC);
        out << " a=";
        print_hex_byte(out, cpu_.A);
        out << " x=";
        print_hex_byte(out, cpu_.X);
        out << " y=";
        print_hex_byte(out, cpu_.Y);
        out << " s=";
        print_hex_byte(out, cpu_.S);
        out << " p=";
        print_hex_byte(out, cpu_.P);
        out << " regs=" << register_compare_state(tick_);
        out << ' ';
        print_stack(out);
        out << " bus=";
        print_bus(out);
        out << '\n';
    }

private:
    void process_memory()
    {
        if ((tick_.status & qe6502_status_writing) != 0u)
        {
            memory_[tick_.address] = tick_.bus;
        }
        else
        {
            tick_.bus = memory_[tick_.address];
        }
    }

    void print_stack(std::ostream& out) const
    {
        const std::uint16_t base = stack_window_base(cpu_.S);
        out << "mem[";
        print_hex_word(out, base);
        out << "]={";

        for (std::uint16_t i = 0u; i < 5u; ++i)
        {
            if (i != 0u)
            {
                out << ',';
            }
            print_hex_byte(out, memory_[stack_window_address(base, i)]);
        }

        out << '}';
    }

    void print_bus(std::ostream& out) const
    {
        if ((tick_.status & qe6502_status_writing) != 0u)
        {
            out << "W ";
            print_hex_word(out, tick_.address);
            out << '=';
            print_hex_byte(out, tick_.bus);
        }
        else
        {
            out << "R ";
            print_hex_word(out, tick_.address);
            out << "={";
            print_hex_byte(out, memory_[tick_.address]);
            out << "}";
        }
    }

    qe6502_t cpu_ = {};
    qe6502_tick_t tick_ = {};
    std::array<std::uint8_t, MemorySize> memory_ = {};
    std::uint64_t cycle_ = 0u;
};

} // namespace

bool run_qe_trace(std::ostream& out, const TraceScript& script, std::string& error)
{
    QeTraceMachine machine;
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
                    machine.step();
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
                machine.reset();
                machine.print_event_state(out, "reset");
                break;
        }
    }

    return true;
}

} // namespace qe6502_trace
