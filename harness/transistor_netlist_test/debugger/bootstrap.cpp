#include "bootstrap.hpp"

#include <cstddef>
#include <cstdio>

namespace perfect6502_debug
{
namespace
{
constexpr std::uint16_t ResetVector = 0xfffcu;
constexpr std::uint8_t SetupDataDone = 0u;
constexpr std::uint8_t SetupDataA = 1u;
constexpr std::uint8_t SetupDataX = 2u;
constexpr std::uint8_t SetupDataY = 3u;
constexpr std::uint8_t SetupDataP = 4u;
constexpr std::uint8_t SetupDataS = 5u;
constexpr std::uint8_t SetupDataPcl = 6u;
constexpr std::uint8_t SetupDataPch = 7u;
constexpr std::uint8_t SetupDataRtiSp = 8u;
constexpr std::uint8_t SetupDataDefaultValue = 0x5au;
constexpr std::uint16_t SetupProgramSize = 35u;
constexpr std::uint32_t SetupCyclesFromDoneAccess = 9u;

std::uint8_t clean_p(std::uint8_t p)
{
    return static_cast<std::uint8_t>((p & 0xcfu) | 0x20u);
}

void put16(PerfectMachine& machine, std::uint16_t address, std::uint16_t value)
{
    machine.write_memory(address, static_cast<std::uint8_t>(value & 0x00ffu));
    machine.write_memory(static_cast<std::uint16_t>(address + 1u),
                         static_cast<std::uint8_t>((value >> 8u) & 0x00ffu));
}

void emit_abs(PerfectMachine& machine,
              std::uint16_t& pc,
              std::uint8_t opcode,
              std::uint16_t address)
{
    machine.write_memory(pc, opcode);
    pc = static_cast<std::uint16_t>(pc + 1u);
    machine.write_memory(pc, static_cast<std::uint8_t>(address & 0x00ffu));
    pc = static_cast<std::uint16_t>(pc + 1u);
    machine.write_memory(pc, static_cast<std::uint8_t>((address >> 8u) & 0x00ffu));
    pc = static_cast<std::uint16_t>(pc + 1u);
}

std::string format_registers(const CpuRegisters& regs)
{
    char buffer[96] = {};

    (void)std::snprintf(buffer,
                        sizeof(buffer),
                        "PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X IR=%02X",
                        static_cast<unsigned>(regs.pc),
                        static_cast<unsigned>(regs.a),
                        static_cast<unsigned>(regs.x),
                        static_cast<unsigned>(regs.y),
                        static_cast<unsigned>(regs.s),
                        static_cast<unsigned>(clean_p(regs.p)),
                        static_cast<unsigned>(regs.ir));

    return std::string(buffer);
}

bool registers_match(const CpuRegisters& got, const CpuRegisters& expected)
{
    return (got.pc == expected.pc) &&
           (got.a == expected.a) &&
           (got.x == expected.x) &&
           (got.y == expected.y) &&
           (got.s == expected.s) &&
           (clean_p(got.p) == clean_p(expected.p));
}

bool inject_setup_program(PerfectMachine& machine,
                          std::uint16_t code_address,
                          std::uint16_t data_address,
                          const CpuRegisters& target_registers,
                          std::string& error)
{
    std::uint16_t pc = code_address;

    if ((static_cast<std::uint32_t>(code_address) +
         static_cast<std::uint32_t>(SetupProgramSize) - 1u) >
        static_cast<std::uint32_t>(SetupCodeEnd))
    {
        error = "setup program does not fit in the reserved setup code area";
        return false;
    }

    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataDone),
                         SetupDataDefaultValue);
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataA),
                         target_registers.a);
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataX),
                         target_registers.x);
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataY),
                         target_registers.y);
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataP),
                         clean_p(target_registers.p));
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataS),
                         target_registers.s);
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataPcl),
                         static_cast<std::uint8_t>(target_registers.pc & 0x00ffu));
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataPch),
                         static_cast<std::uint8_t>((target_registers.pc >> 8u) & 0x00ffu));
    machine.write_memory(static_cast<std::uint16_t>(data_address + SetupDataRtiSp),
                         static_cast<std::uint8_t>(target_registers.s - 3u));

    put16(machine, ResetVector, code_address);

    /* LDX data.rti_sp */
    emit_abs(machine, pc, 0xaeu, static_cast<std::uint16_t>(data_address + SetupDataRtiSp));
    /* TXS */
    machine.write_memory(pc, 0x9au);
    pc = static_cast<std::uint16_t>(pc + 1u);

    /*
       Stack frame for RTI: P, PCL, PCH at (S-2), (S-1), S.
       Do not use absolute,X here: 6502 stack pulls wrap inside page $01,
       while absolute,X does not wrap from $01FF to $0100.
    */
    emit_abs(machine, pc, 0xadu, static_cast<std::uint16_t>(data_address + SetupDataP));
    emit_abs(machine, pc, 0x8du, static_cast<std::uint16_t>(0x0100u | static_cast<std::uint8_t>(target_registers.s - 2u)));

    emit_abs(machine, pc, 0xadu, static_cast<std::uint16_t>(data_address + SetupDataPcl));
    emit_abs(machine, pc, 0x8du, static_cast<std::uint16_t>(0x0100u | static_cast<std::uint8_t>(target_registers.s - 1u)));

    emit_abs(machine, pc, 0xadu, static_cast<std::uint16_t>(data_address + SetupDataPch));
    emit_abs(machine, pc, 0x8du, static_cast<std::uint16_t>(0x0100u | target_registers.s));

    /* final A/X/Y */
    emit_abs(machine, pc, 0xadu, static_cast<std::uint16_t>(data_address + SetupDataA));
    emit_abs(machine, pc, 0xaeu, static_cast<std::uint16_t>(data_address + SetupDataX));
    emit_abs(machine, pc, 0xacu, static_cast<std::uint16_t>(data_address + SetupDataY));

    /* marker and transfer to final PC/P/S */
    emit_abs(machine, pc, 0xeeu, static_cast<std::uint16_t>(data_address + SetupDataDone));
    machine.write_memory(pc, 0x40u); /* RTI */
    pc = static_cast<std::uint16_t>(pc + 1u);

    if (static_cast<std::uint16_t>(pc - code_address) != SetupProgramSize)
    {
        error = "internal setup program size mismatch";
        return false;
    }

    return true;
}

bool wait_for_opcode_fetch(PerfectMachine& machine,
                           std::uint16_t fetch_pc,
                           std::uint32_t max_half_cycles,
                           std::string& error)
{
    for (std::uint32_t i = 0u; i < max_half_cycles; ++i)
    {
        if (machine.is_read() &&
            (machine.read_address_bus() == fetch_pc) &&
            (machine.read_registers().pc == fetch_pc))
        {
            return true;
        }

        machine.step_half_cycle();
    }

    char buffer[192] = {};
    const CpuRegisters regs = machine.read_registers();

    (void)std::snprintf(buffer,
                        sizeof(buffer),
                        "timeout waiting for opcode fetch: expected_pc=%04X got[%s] AB=%04X DB=%02X RW=%u half=%llu",
                        static_cast<unsigned>(fetch_pc),
                        format_registers(regs).c_str(),
                        static_cast<unsigned>(machine.read_address_bus()),
                        static_cast<unsigned>(machine.read_data_bus()),
                        machine.is_read() ? 1u : 0u,
                        static_cast<unsigned long long>(machine.half_cycle()));

    error = buffer;
    return false;
}

bool run_setup_program(PerfectMachine& machine,
                       std::uint16_t code_address,
                       std::uint16_t data_address,
                       const CpuRegisters& target_registers,
                       std::string& error)
{
    bool seen_marker = false;
    std::uint32_t cycles_after_marker = 0u;

    if (!wait_for_opcode_fetch(machine, code_address, 256u, error))
    {
        return false;
    }

    for (std::uint32_t i = 0u; i < 512u; ++i)
    {
        if (!seen_marker && (machine.read_address_bus() == data_address))
        {
            seen_marker = true;
        }

        machine.step_full_cycle();

        if (seen_marker)
        {
            ++cycles_after_marker;
            if (cycles_after_marker == SetupCyclesFromDoneAccess)
            {
                const std::uint8_t expected_marker = static_cast<std::uint8_t>(SetupDataDefaultValue + 1u);
                const std::uint8_t actual_marker = machine.read_memory(data_address);

                if (actual_marker != expected_marker)
                {
                    char buffer[192] = {};
                    (void)std::snprintf(buffer,
                                        sizeof(buffer),
                                        "setup marker mismatch: data=%04X got=%02X expected=%02X",
                                        static_cast<unsigned>(data_address),
                                        static_cast<unsigned>(actual_marker),
                                        static_cast<unsigned>(expected_marker));
                    error = buffer;
                    return false;
                }

                const CpuRegisters actual_registers = machine.read_registers();
                if (!registers_match(actual_registers, target_registers))
                {
                    error = "setup register mismatch: got[" + format_registers(actual_registers) +
                            "] expected[" + format_registers(target_registers) + "]";
                    return false;
                }

                return true;
            }
        }
    }

    error = "setup timeout";
    return false;
}

} // namespace

bool bootstrap_to_registers(PerfectMachine& machine,
                            std::uint16_t code_address,
                            std::uint16_t data_address,
                            const CpuRegisters& target_registers,
                            std::string& error)
{
    if (!inject_setup_program(machine, code_address, data_address, target_registers, error))
    {
        return false;
    }

    if (!machine.reset(error))
    {
        return false;
    }

    return run_setup_program(machine, code_address, data_address, target_registers, error);
}

} // namespace perfect6502_debug
