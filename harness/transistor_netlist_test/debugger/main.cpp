#include "bootstrap.hpp"
#include "perfect_bridge.hpp"

#include <cstdint>
#include <cstdio>
#include <string>

namespace
{
constexpr std::uint16_t ProgramAddress = 0x0200u;
constexpr std::uint8_t InitialA = 0x41u;
constexpr std::uint8_t ExpectedA = static_cast<std::uint8_t>(InitialA + 1u);

void write_increment_a_program(perfect6502_debug::PerfectMachine& machine)
{
    machine.write_memory(ProgramAddress, 0x18u);                         /* CLC */
    machine.write_memory(static_cast<std::uint16_t>(ProgramAddress + 1u), 0x69u); /* ADC #$01 */
    machine.write_memory(static_cast<std::uint16_t>(ProgramAddress + 2u), 0x01u);
    machine.write_memory(static_cast<std::uint16_t>(ProgramAddress + 3u), 0xeau); /* NOP */
}

perfect6502_debug::CpuRegisters make_initial_registers()
{
    perfect6502_debug::CpuRegisters regs;

    regs.pc = ProgramAddress;
    regs.a = InitialA;
    regs.x = 0x12u;
    regs.y = 0x34u;
    regs.s = 0xfdu;
    regs.p = 0x24u;

    return regs;
}

} // namespace

int main()
{
    perfect6502_debug::PerfectMachine machine;
    std::string error;

    machine.fill_memory(0xeau);
    write_increment_a_program(machine);

    const perfect6502_debug::CpuRegisters initial_registers = make_initial_registers();
    if (!perfect6502_debug::bootstrap_to_registers(machine,
                                                   perfect6502_debug::SetupCodeAddress,
                                                   perfect6502_debug::SetupDataAddress,
                                                   initial_registers,
                                                   error))
    {
        (void)std::fprintf(stderr, "perfect6502_debug bootstrap failed: %s\n", error.c_str());
        return 1;
    }

    for (std::uint32_t i = 0u; i < 5u; ++i)
    {
        machine.step_full_cycle();
    }

    const perfect6502_debug::CpuRegisters final_registers = machine.read_registers();
    if (final_registers.a != ExpectedA)
    {
        (void)std::fprintf(stderr,
                           "perfect6502_debug smoke failed: A=%02X expected=%02X PC=%04X half=%llu\n",
                           static_cast<unsigned>(final_registers.a),
                           static_cast<unsigned>(ExpectedA),
                           static_cast<unsigned>(final_registers.pc),
                           static_cast<unsigned long long>(machine.half_cycle()));
        return 1;
    }

    (void)std::printf("perfect6502_debug smoke PASS: A=%02X PC=%04X half=%llu\n",
                      static_cast<unsigned>(final_registers.a),
                      static_cast<unsigned>(final_registers.pc),
                      static_cast<unsigned long long>(machine.half_cycle()));
    return 0;
}
