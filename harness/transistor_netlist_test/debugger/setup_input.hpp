#ifndef PERFECT6502_DEBUG_SETUP_INPUT_HPP
#define PERFECT6502_DEBUG_SETUP_INPUT_HPP

#include "perfect_bridge.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace perfect6502_debug
{

struct MemoryWrite
{
    std::uint16_t address = 0u;
    std::vector<std::uint8_t> bytes;
};

struct SetupInput
{
    SetupInput();

    CpuRegisters registers;
    std::uint8_t memory_fill_value = 0xeau;
    std::vector<MemoryWrite> memory_writes;
};

bool parse_setup_input(const std::string& text, SetupInput& setup, std::string& error);
bool apply_setup_memory(PerfectMachine& machine, const SetupInput& setup, std::string& error);

} // namespace perfect6502_debug

#endif
