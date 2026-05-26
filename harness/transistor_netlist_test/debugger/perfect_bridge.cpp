#include "perfect_bridge.hpp"

extern "C"
{
#include "../third_party/perfect6502/perfect6502.h"
#include "../third_party/perfect6502/types.h"
#include "../third_party/perfect6502/netlist_sim.h"
}

#include <algorithm>
#include <cstddef>
#include <utility>

namespace perfect6502_debug
{
namespace
{
constexpr std::size_t MemorySize = 65536u;
constexpr nodenum_t IrqNode = 103;
constexpr nodenum_t NmiNode = 1297;

bool is_active_low_node_asserted(void* cpu, nodenum_t node)
{
    return isNodeHigh(static_cast<state_t*>(cpu), node) == 0;
}

void set_active_low_node_asserted(void* cpu, nodenum_t node, bool asserted)
{
    setNode(static_cast<state_t*>(cpu), node, asserted ? 0 : 1);
}
}

PerfectSnapshot::PerfectSnapshot(void* snapshot) :
    snapshot_(snapshot)
{
}

PerfectSnapshot::~PerfectSnapshot()
{
    destroy();
}

PerfectSnapshot::PerfectSnapshot(PerfectSnapshot&& other) noexcept :
    snapshot_(other.snapshot_)
{
    other.snapshot_ = nullptr;
}

PerfectSnapshot& PerfectSnapshot::operator=(PerfectSnapshot&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        snapshot_ = other.snapshot_;
        other.snapshot_ = nullptr;
    }

    return *this;
}

bool PerfectSnapshot::is_valid() const
{
    return snapshot_ != nullptr;
}

void PerfectSnapshot::destroy()
{
    if (snapshot_ != nullptr)
    {
        perfect6502_snapshot_destroy(static_cast<perfect6502_snapshot_t*>(snapshot_));
        snapshot_ = nullptr;
    }
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

PerfectSnapshot PerfectMachine::create_snapshot(std::string& error) const
{
    if (cpu_ == nullptr)
    {
        error = "cannot snapshot an uninitialized perfect6502 machine";
        return PerfectSnapshot();
    }

    perfect6502_snapshot_t* snapshot = perfect6502_snapshot_create(cpu_);
    if (snapshot == nullptr)
    {
        error = "perfect6502_snapshot_create failed";
        return PerfectSnapshot();
    }

    return PerfectSnapshot(snapshot);
}

bool PerfectMachine::restore_snapshot(const PerfectSnapshot& snapshot, std::string& error)
{
    if (cpu_ == nullptr)
    {
        error = "cannot restore into an uninitialized perfect6502 machine";
        return false;
    }

    if (!snapshot.is_valid())
    {
        error = "cannot restore from an empty perfect6502 snapshot";
        return false;
    }

    if (!perfect6502_snapshot_restore(cpu_, static_cast<const perfect6502_snapshot_t*>(snapshot.snapshot_)))
    {
        error = "perfect6502_snapshot_restore failed";
        return false;
    }

    return true;
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

InterruptInputs PerfectMachine::read_interrupt_inputs() const
{
    InterruptInputs inputs;
    inputs.irq_asserted = is_active_low_node_asserted(cpu_, IrqNode);
    inputs.nmi_asserted = is_active_low_node_asserted(cpu_, NmiNode);
    return inputs;
}

void PerfectMachine::set_irq_asserted(bool asserted)
{
    set_active_low_node_asserted(cpu_, IrqNode, asserted);
}

void PerfectMachine::set_nmi_asserted(bool asserted)
{
    set_active_low_node_asserted(cpu_, NmiNode, asserted);
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
