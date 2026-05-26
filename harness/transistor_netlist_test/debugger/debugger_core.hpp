#ifndef PERFECT6502_DEBUG_DEBUGGER_CORE_HPP
#define PERFECT6502_DEBUG_DEBUGGER_CORE_HPP

#include "cycle_cursor.hpp"
#include "perfect_bridge.hpp"
#include "setup_input.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace perfect6502_debug
{

enum class DebuggerCycleMode
{
    Halfcycle,
    Fullcycle
};

struct BusStateView
{
    std::uint16_t address = 0u;
    std::uint8_t data = 0u;
    std::uint8_t memory_data = 0u;
    bool read = true;
    bool served = false;
    bool memory_data_valid = false;
};

struct CpuPointView
{
    CycleCursor cursor;
    std::uint64_t perfect_half_cycle = 0u;
    CpuRegisters registers;
    BusStateView bus;
    InterruptInputs interrupt_inputs;
};

struct FullCycleTransitionView
{
    std::uint64_t cycle_number = 0u;
    CpuPointView cpu_input;
    CpuPointView cpu_output;
    CpuPointView next_cpu_input;
};

struct MemoryByteView
{
    std::uint16_t address = 0u;
    std::uint8_t value = 0u;
    bool marker = false;
};

struct DebuggerView
{
    DebuggerCycleMode cycle_mode = DebuggerCycleMode::Halfcycle;
    CpuPointView point;
    std::vector<MemoryByteView> stack_bytes;
    std::vector<MemoryByteView> pc_bytes;
};

class DebuggerCore
{
public:
    DebuggerCore() = default;

    bool initialize(std::string& error);
    bool apply_setup_text(const std::string& text, std::string& error);

    bool step_halfcycles(std::uint32_t count, std::string& error);
    bool step_full_cycles(std::uint32_t count,
                          std::vector<FullCycleTransitionView>& transitions,
                          std::string& error);
    bool undo(std::uint32_t count, std::string& error);

    bool set_fullcycle_mode(bool enabled, std::string& error);
    DebuggerCycleMode cycle_mode() const;

    bool set_irq_asserted(bool asserted, std::string& error);
    bool set_nmi_asserted(bool asserted, std::string& error);
    bool toggle_nmi(std::string& error);

    DebuggerView make_view() const;

private:
    struct UndoPoint
    {
        PerfectSnapshot snapshot;
        CycleCursor cursor;
        DebuggerCycleMode cycle_mode = DebuggerCycleMode::Halfcycle;
    };

    bool apply_parsed_setup(const SetupInput& setup, std::string& error);
    bool save_undo_point(std::string& error);
    void trim_undo_history();
    void clear_undo_history();

    void step_one_halfcycle();
    bool align_to_fullcycle_cpu_input_boundary(std::string& error);
    CpuPointView make_cpu_point(bool bus_served) const;

    PerfectMachine machine_;
    CycleCursor cursor_;
    DebuggerCycleMode cycle_mode_ = DebuggerCycleMode::Fullcycle;
    std::deque<UndoPoint> undo_points_;
};

} // namespace perfect6502_debug

#endif
