#ifndef PERFECT6502_DEBUG_BOOTSTRAP_HPP
#define PERFECT6502_DEBUG_BOOTSTRAP_HPP

#include "perfect_bridge.hpp"

#include <cstdint>
#include <string>

namespace perfect6502_debug
{

constexpr std::uint16_t SetupCodeAddress = 0xff10u;
constexpr std::uint16_t SetupCodeEnd = 0xff80u;
constexpr std::uint16_t SetupDataAddress = 0xff00u;

bool bootstrap_to_registers(PerfectMachine& machine,
                            std::uint16_t code_address,
                            std::uint16_t data_address,
                            const CpuRegisters& target_registers,
                            std::string& error);

} // namespace perfect6502_debug

#endif
