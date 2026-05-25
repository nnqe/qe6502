#ifndef PERFECT6502_DEBUG_PERFECT_BRIDGE_HPP
#define PERFECT6502_DEBUG_PERFECT_BRIDGE_HPP

#include <cstdint>
#include <string>

namespace perfect6502_debug
{

struct CpuRegisters
{
    std::uint16_t pc = 0u;
    std::uint8_t a = 0u;
    std::uint8_t x = 0u;
    std::uint8_t y = 0u;
    std::uint8_t s = 0u;
    std::uint8_t p = 0u;
    std::uint8_t ir = 0u;
};

class PerfectSnapshot
{
public:
    PerfectSnapshot() = default;
    ~PerfectSnapshot();

    PerfectSnapshot(const PerfectSnapshot&) = delete;
    PerfectSnapshot& operator=(const PerfectSnapshot&) = delete;

    PerfectSnapshot(PerfectSnapshot&& other) noexcept;
    PerfectSnapshot& operator=(PerfectSnapshot&& other) noexcept;

    bool is_valid() const;
    void destroy();

private:
    explicit PerfectSnapshot(void* snapshot);

    void* snapshot_ = nullptr;

    friend class PerfectMachine;
};

class PerfectMachine
{
public:
    PerfectMachine() = default;
    ~PerfectMachine();

    PerfectMachine(const PerfectMachine&) = delete;
    PerfectMachine& operator=(const PerfectMachine&) = delete;

    bool reset(std::string& error);
    void destroy();

    bool is_valid() const;

    PerfectSnapshot create_snapshot(std::string& error) const;
    bool restore_snapshot(const PerfectSnapshot& snapshot, std::string& error);

    void step_half_cycle();
    void step_full_cycle();

    CpuRegisters read_registers() const;
    std::uint16_t read_address_bus() const;
    std::uint8_t read_data_bus() const;
    bool is_read() const;
    std::uint64_t half_cycle() const;

    std::uint8_t read_memory(std::uint16_t address) const;
    void write_memory(std::uint16_t address, std::uint8_t value);
    void fill_memory(std::uint8_t value);

private:
    void* cpu_ = nullptr;
};

} // namespace perfect6502_debug

#endif
