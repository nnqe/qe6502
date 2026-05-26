#include "debugger_core.hpp"

#include "bootstrap.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace perfect6502_debug
{
namespace
{
constexpr std::size_t MaxUndoPoints = 512u;
constexpr int StackBytesBefore = 4;
constexpr int StackBytesAfter = 4;
constexpr std::uint16_t StackPage = 0x0100u;
constexpr std::uint32_t StackByteMask = 0xffu;
constexpr std::uint32_t PcByteCount = 8u;

std::uint16_t stack_address(std::uint8_t stack_pointer, int offset)
{
    const int stack_index = (static_cast<int>(stack_pointer) + offset) & static_cast<int>(StackByteMask);
    return static_cast<std::uint16_t>(StackPage | static_cast<std::uint16_t>(stack_index));
}

} // namespace

bool DebuggerCore::initialize(std::string& error)
{
    SetupInput setup;
    return apply_parsed_setup(setup, error);
}

bool DebuggerCore::apply_setup_text(const std::string& text, std::string& error)
{
    SetupInput setup;
    if (!parse_setup_input(text, setup, error))
    {
        return false;
    }

    return apply_parsed_setup(setup, error);
}

bool DebuggerCore::step_halfcycles(std::uint32_t count, std::string& error)
{
    if (cycle_mode_ == DebuggerCycleMode::Fullcycle)
    {
        error = "half is unavailable while fullcycle mode is on; use cycle or fullcycle off";
        return false;
    }

    if (count == 0u)
    {
        error = "half requires a positive count";
        return false;
    }

    if (!save_undo_point(error))
    {
        return false;
    }

    for (std::uint32_t i = 0u; i < count; ++i)
    {
        step_one_halfcycle();
    }

    return true;
}

bool DebuggerCore::step_full_cycles(std::uint32_t count,
                                    std::vector<FullCycleTransitionView>& transitions,
                                    std::string& error)
{
    transitions.clear();

    if (count == 0u)
    {
        error = "cycle requires a positive count";
        return false;
    }

    if (cycle_mode_ == DebuggerCycleMode::Fullcycle)
    {
        if (cursor_.boundary != CycleBoundary::BeforeCpuHalf)
        {
            error = "fullcycle mode invariant broken: expected boundary=BeforeCpuHalf";
            return false;
        }

        if (!save_undo_point(error))
        {
            return false;
        }

        transitions.reserve(count);

        /*
            Fullcycle mode presents the debugger at the CPU-input boundary.

            The visible position is after the Memory halfcycle and before the CPU halfcycle.
            At this point the pending bus request has already been serviced by memory, so the
            displayed data is the actual data that the next CPU halfcycle will consume or the
            actual write that memory just observed.

            A user-visible `cycle` command therefore does not run M+C from the outside boundary.
            It executes the remaining CPU halfcycle of the displayed full cycle, captures the
            CPU state and output bus request left by that CPU halfcycle, then secretly executes
            the next Memory halfcycle and captures the next CPU-input boundary. The hidden
            Memory halfcycle is real execution, not a skipped step. It is hidden only from the
            interactive command model so the user can reason mostly in full cycles while still
            seeing complete input data for each CPU halfcycle.
        */
        for (std::uint32_t i = 0u; i < count; ++i)
        {
            FullCycleTransitionView transition;
            transition.cycle_number = cursor_.cycle_number;
            transition.cpu_input = make_cpu_point(true);

            step_one_halfcycle();
            transition.cpu_output = make_cpu_point(false);

            step_one_halfcycle();
            transition.next_cpu_input = make_cpu_point(true);

            transitions.push_back(transition);
        }

        return true;
    }

    if (!is_full_cycle_boundary(cursor_))
    {
        error = "cycle requires boundary=BeforeMemoryHalf; use half to execute the pending CPU halfcycle first";
        return false;
    }

    if (!save_undo_point(error))
    {
        return false;
    }

    for (std::uint32_t i = 0u; i < count; ++i)
    {
        step_one_halfcycle();
        step_one_halfcycle();
    }

    return true;
}

bool DebuggerCore::undo(std::uint32_t count, std::string& error)
{
    if (count == 0u)
    {
        error = "undo requires a positive count";
        return false;
    }

    if (count > undo_points_.size())
    {
        error = "not enough undo history";
        return false;
    }

    for (std::uint32_t i = 0u; i < count; ++i)
    {
        UndoPoint& point = undo_points_.back();
        if (!machine_.restore_snapshot(point.snapshot, error))
        {
            return false;
        }

        cursor_ = point.cursor;
        cycle_mode_ = point.cycle_mode;
        undo_points_.pop_back();
    }

    return true;
}

bool DebuggerCore::set_fullcycle_mode(bool enabled, std::string& error)
{
    if (!enabled)
    {
        cycle_mode_ = DebuggerCycleMode::Halfcycle;
        return true;
    }

    if (cycle_mode_ == DebuggerCycleMode::Fullcycle)
    {
        return true;
    }

    if (cursor_.boundary == CycleBoundary::BeforeCpuHalf)
    {
        cycle_mode_ = DebuggerCycleMode::Fullcycle;
        return true;
    }

    if (!save_undo_point(error))
    {
        return false;
    }

    step_one_halfcycle();
    cycle_mode_ = DebuggerCycleMode::Fullcycle;
    return true;
}

DebuggerCycleMode DebuggerCore::cycle_mode() const
{
    return cycle_mode_;
}

bool DebuggerCore::set_irq_asserted(bool asserted, std::string& error)
{
    if (!save_undo_point(error))
    {
        return false;
    }

    machine_.set_irq_asserted(asserted);
    return true;
}

bool DebuggerCore::set_nmi_asserted(bool asserted, std::string& error)
{
    if (!save_undo_point(error))
    {
        return false;
    }

    machine_.set_nmi_asserted(asserted);
    return true;
}

bool DebuggerCore::toggle_nmi(std::string& error)
{
    const InterruptInputs inputs = machine_.read_interrupt_inputs();
    return set_nmi_asserted(!inputs.nmi_asserted, error);
}

DebuggerView DebuggerCore::make_view() const
{
    DebuggerView view;
    view.cycle_mode = cycle_mode_;
    view.point = make_cpu_point(cursor_.boundary == CycleBoundary::BeforeCpuHalf);

    const CpuRegisters& regs = view.point.registers;

    for (int offset = -StackBytesBefore; offset <= StackBytesAfter; ++offset)
    {
        MemoryByteView byte;
        byte.address = stack_address(regs.s, offset);
        byte.value = machine_.read_memory(byte.address);
        byte.marker = (offset == 0);
        view.stack_bytes.push_back(byte);
    }

    for (std::uint32_t i = 0u; i < PcByteCount; ++i)
    {
        MemoryByteView byte;
        byte.address = static_cast<std::uint16_t>(regs.pc + i);
        byte.value = machine_.read_memory(byte.address);
        byte.marker = (i == 0u);
        view.pc_bytes.push_back(byte);
    }

    return view;
}

bool DebuggerCore::apply_parsed_setup(const SetupInput& setup, std::string& error)
{
    if (!apply_setup_memory(machine_, setup, error))
    {
        return false;
    }

    if (!bootstrap_to_registers(machine_,
                                SetupCodeAddress,
                                SetupDataAddress,
                                setup.registers,
                                error))
    {
        return false;
    }

    cursor_ = CycleCursor();
    clear_undo_history();

    if (cycle_mode_ == DebuggerCycleMode::Fullcycle)
    {
        return align_to_fullcycle_cpu_input_boundary(error);
    }

    return true;
}

bool DebuggerCore::save_undo_point(std::string& error)
{
    PerfectSnapshot snapshot = machine_.create_snapshot(error);
    if (!snapshot.is_valid())
    {
        return false;
    }

    UndoPoint point;
    point.snapshot = std::move(snapshot);
    point.cursor = cursor_;
    point.cycle_mode = cycle_mode_;
    undo_points_.push_back(std::move(point));
    trim_undo_history();
    return true;
}

void DebuggerCore::trim_undo_history()
{
    while (undo_points_.size() > MaxUndoPoints)
    {
        undo_points_.pop_front();
    }
}

void DebuggerCore::clear_undo_history()
{
    undo_points_.clear();
}

void DebuggerCore::step_one_halfcycle()
{
    machine_.step_half_cycle();
    advance_one_halfcycle(cursor_);
}

bool DebuggerCore::align_to_fullcycle_cpu_input_boundary(std::string& error)
{
    if (cursor_.boundary == CycleBoundary::BeforeCpuHalf)
    {
        return true;
    }

    if (cursor_.boundary != CycleBoundary::BeforeMemoryHalf)
    {
        error = "unknown cycle boundary";
        return false;
    }

    step_one_halfcycle();
    return true;
}

CpuPointView DebuggerCore::make_cpu_point(bool bus_served) const
{
    CpuPointView point;
    point.cursor = cursor_;
    point.perfect_half_cycle = machine_.half_cycle();
    point.registers = machine_.read_registers();
    point.bus.address = machine_.read_address_bus();
    point.bus.data = machine_.read_data_bus();
    point.bus.read = machine_.is_read();
    point.bus.served = bus_served;
    point.bus.memory_data = machine_.read_memory(point.bus.address);
    point.bus.memory_data_valid = true;
    point.interrupt_inputs = machine_.read_interrupt_inputs();
    return point;
}

} // namespace perfect6502_debug
