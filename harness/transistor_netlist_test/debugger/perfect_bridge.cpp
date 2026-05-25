#include "perfect_bridge.hpp"

extern "C"
{
#include "../third_party/perfect6502/perfect6502.h"
}

#include <algorithm>
#include <cstddef>

namespace perfect6502_debug
{
namespace
{
constexpr std::size_t MemorySize = 65536u;
}

PerfectMachine::~PerfectMachine()
{
    destroy();
}

bool PerfectMachine::reset(std::string& error)
{
    destroy();

    cpu_ = initAndResetChip();
    if (cpu_ == nullptr)
    {
        error = "initAndResetChip failed";
        return false;
    }

    return true;
}

void PerfectMachine::destroy()
{
    if (cpu_ != nullptr)
    {
        destroyChip(cpu_);
        cpu_ = nullptr;
    }
}

bool PerfectMachine::is_valid() const
{
    return cpu_ != nullptr;
}

void PerfectMachine::step_half_cycle()
{
    step(cpu_);
}

void PerfectMachine::step_full_cycle()
{
    step_half_cycle();
    step_half_cycle();
}

CpuRegisters PerfectMachine::read_registers() const
{
    CpuRegisters regs;

    regs.pc = readPC(cpu_);
    regs.a = readA(cpu_);
    regs.x = readX(cpu_);
    regs.y = readY(cpu_);
    regs.s = readSP(cpu_);
    regs.p = readP(cpu_);
    regs.ir = readIR(cpu_);

    return regs;
}

std::uint16_t PerfectMachine::read_address_bus() const
{
    return readAddressBus(cpu_);
}

std::uint8_t PerfectMachine::read_data_bus() const
{
    return readDataBus(cpu_);
}

bool PerfectMachine::is_read() const
{
    return readRW(cpu_) != 0u;
}

std::uint64_t PerfectMachine::half_cycle() const
{
    return static_cast<std::uint64_t>(cycle);
}

std::uint8_t PerfectMachine::read_memory(std::uint16_t address) const
{
    return memory[address];
}

void PerfectMachine::write_memory(std::uint16_t address, std::uint8_t value)
{
    memory[address] = value;
}

void PerfectMachine::fill_memory(std::uint8_t value)
{
    std::fill(memory, memory + MemorySize, value);
}

} // namespace perfect6502_debug
