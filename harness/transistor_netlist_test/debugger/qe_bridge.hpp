#ifndef PERFECT6502_DEBUG_QE_BRIDGE_HPP
#define PERFECT6502_DEBUG_QE_BRIDGE_HPP

#include "perfect_bridge.hpp"

#include <array>
#include <cstdint>
#include <string>

extern "C"
{
#include "qe6502.h"
}

namespace perfect6502_debug
{

struct SetupInput;

struct QeSnapshot
{
    qe6502_t cpu = {};
    qe6502_tick_t tick = {};
    std::array<std::uint8_t, 65536u> memory = {};
    std::uint8_t data_bus = 0u;
    std::uint8_t ir = 0u;
    std::uint64_t half_cycle = 0u;
    bool before_memory_half = true;
    CpuRegisters visible_registers;
    bool side_effects_pending = false;
    bool valid = false;
};

class QeMachine
{
public:
    QeMachine();

    bool apply_setup(const SetupInput& setup, std::string& error);

    QeSnapshot create_snapshot(std::string& error) const;
    bool restore_snapshot(const QeSnapshot& snapshot, std::string& error);

    void step_half_cycle();
    void step_full_cycle();

    CpuRegisters read_registers() const;
    std::uint16_t read_address_bus() const;
    std::uint8_t read_data_bus() const;
    bool is_read() const;
    std::uint64_t half_cycle() const;

    InterruptInputs read_interrupt_inputs() const;
    void set_irq_asserted(bool asserted);
    void set_nmi_asserted(bool asserted);

    std::uint8_t read_memory(std::uint16_t address) const;
    void write_memory(std::uint16_t address, std::uint8_t value);
    void fill_memory(std::uint8_t value);

private:
    void start_at_pc(std::uint16_t pc);
    void publish_instruction_side_effects();

    qe6502_t cpu_ = {};
    qe6502_tick_t tick_ = {};
    std::array<std::uint8_t, 65536u> memory_ = {};
    std::uint8_t data_bus_ = 0u;
    std::uint8_t ir_ = 0u;
    CpuRegisters visible_registers_;
    bool side_effects_pending_ = false;
    std::uint64_t half_cycle_ = 0u;
    bool before_memory_half_ = true;
    bool initialized_ = false;
};

} // namespace perfect6502_debug

#endif
