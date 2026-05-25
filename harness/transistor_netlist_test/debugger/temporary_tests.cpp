#include "temporary_tests.hpp"

#include "bootstrap.hpp"
#include "perfect_bridge.hpp"
#include "setup_input.hpp"

#include <cstdint>
#include <cstdio>
#include <string>

namespace perfect6502_debug
{
namespace
{
constexpr std::uint16_t ProgramAddress = 0x0200u;
constexpr std::uint16_t SnapshotMemoryProbe = 0x1234u;
constexpr std::uint8_t InitialA = 0x41u;
constexpr std::uint8_t ExpectedA = static_cast<std::uint8_t>(InitialA + 1u);
constexpr std::uint8_t OriginalProbeValue = 0x5au;
constexpr std::uint8_t MutatedProbeValue = 0xa5u;

const char* increment_a_setup_text()
{
    return
        "// setup parser smoke coverage\n"
        "pc = 0x0200; a = 0x41; x = 4; x = 0x12; y = 52; s = 0xfd; "
        "p = b00100100;\n"
        "mem[0x0200] = { 0xff, 0xff, 0xff, 0xff };\n"
        "mem[0x0200] = { 0x18, 0x69, 0x01, 0xea }; // CLC; ADC #$01; NOP\n";
}

bool parse_increment_setup(SetupInput& setup, std::string& error)
{
    if (!parse_setup_input(increment_a_setup_text(), setup, error))
    {
        return false;
    }

    if ((setup.registers.pc != ProgramAddress) ||
        (setup.registers.a != InitialA) ||
        (setup.registers.x != 0x12u) ||
        (setup.registers.y != 0x34u) ||
        (setup.registers.s != 0xfdu) ||
        (setup.registers.p != 0x24u) ||
        (setup.memory_writes.size() != 2u))
    {
        error = "parsed setup state does not match the expected smoke-test setup";
        return false;
    }

    return true;
}

bool prepare_bootstrapped_increment_program(PerfectMachine& machine, std::string& error)
{
    SetupInput setup;
    if (!parse_increment_setup(setup, error))
    {
        return false;
    }

    if (!apply_setup_memory(machine, setup, error))
    {
        return false;
    }

    return bootstrap_to_registers(machine,
                                  SetupCodeAddress,
                                  SetupDataAddress,
                                  setup.registers,
                                  error);
}

bool run_increment_program(PerfectMachine& machine, std::string& error)
{
    for (std::uint32_t i = 0u; i < 5u; ++i)
    {
        machine.step_full_cycle();
    }

    const CpuRegisters final_registers = machine.read_registers();
    if (final_registers.a != ExpectedA)
    {
        char buffer[160] = {};
        (void)std::snprintf(buffer,
                            sizeof(buffer),
                            "increment program failed: A=%02X expected=%02X PC=%04X half=%llu",
                            static_cast<unsigned>(final_registers.a),
                            static_cast<unsigned>(ExpectedA),
                            static_cast<unsigned>(final_registers.pc),
                            static_cast<unsigned long long>(machine.half_cycle()));
        error = buffer;
        return false;
    }

    return true;
}

bool verify_restored_bootstrap_state(PerfectMachine& machine,
                                     std::uint64_t expected_half_cycle,
                                     std::string& error)
{
    const CpuRegisters registers = machine.read_registers();
    if ((registers.pc != ProgramAddress) || (registers.a != InitialA))
    {
        char buffer[160] = {};
        (void)std::snprintf(buffer,
                            sizeof(buffer),
                            "snapshot restore failed: PC=%04X A=%02X expected PC=%04X A=%02X",
                            static_cast<unsigned>(registers.pc),
                            static_cast<unsigned>(registers.a),
                            static_cast<unsigned>(ProgramAddress),
                            static_cast<unsigned>(InitialA));
        error = buffer;
        return false;
    }

    if (machine.half_cycle() != expected_half_cycle)
    {
        char buffer[160] = {};
        (void)std::snprintf(buffer,
                            sizeof(buffer),
                            "snapshot cycle restore failed: half=%llu expected=%llu",
                            static_cast<unsigned long long>(machine.half_cycle()),
                            static_cast<unsigned long long>(expected_half_cycle));
        error = buffer;
        return false;
    }

    if (machine.read_memory(SnapshotMemoryProbe) != OriginalProbeValue)
    {
        char buffer[160] = {};
        (void)std::snprintf(buffer,
                            sizeof(buffer),
                            "snapshot memory restore failed: mem[%04X]=%02X expected=%02X",
                            static_cast<unsigned>(SnapshotMemoryProbe),
                            static_cast<unsigned>(machine.read_memory(SnapshotMemoryProbe)),
                            static_cast<unsigned>(OriginalProbeValue));
        error = buffer;
        return false;
    }

    return true;
}

void print_failure(const char* test_name, const char* stage, const std::string& error)
{
    (void)std::fprintf(stderr,
                       "perfect6502_debug %s failed during %s: %s\n",
                       test_name,
                       stage,
                       error.c_str());
}

} // namespace

bool test_bootstrap()
{
    PerfectMachine machine;
    std::string error;

    if (!prepare_bootstrapped_increment_program(machine, error))
    {
        print_failure("bootstrap", "bootstrap", error);
        return false;
    }

    if (!run_increment_program(machine, error))
    {
        print_failure("bootstrap", "increment program", error);
        return false;
    }

    const CpuRegisters final_registers = machine.read_registers();
    (void)std::printf("perfect6502_debug bootstrap PASS: A=%02X PC=%04X half=%llu\n",
                      static_cast<unsigned>(final_registers.a),
                      static_cast<unsigned>(final_registers.pc),
                      static_cast<unsigned long long>(machine.half_cycle()));
    return true;
}

bool test_snapshot_restore()
{
    PerfectMachine machine;
    std::string error;

    if (!prepare_bootstrapped_increment_program(machine, error))
    {
        print_failure("snapshot_restore", "bootstrap", error);
        return false;
    }

    machine.write_memory(SnapshotMemoryProbe, OriginalProbeValue);

    const std::uint64_t snapshot_half_cycle = machine.half_cycle();
    PerfectSnapshot snapshot = machine.create_snapshot(error);
    if (!snapshot.is_valid())
    {
        print_failure("snapshot_restore", "snapshot", error);
        return false;
    }

    machine.write_memory(SnapshotMemoryProbe, MutatedProbeValue);

    if (!run_increment_program(machine, error))
    {
        print_failure("snapshot_restore", "increment program before restore", error);
        return false;
    }

    if (!machine.restore_snapshot(snapshot, error))
    {
        print_failure("snapshot_restore", "restore", error);
        return false;
    }

    if (!verify_restored_bootstrap_state(machine, snapshot_half_cycle, error))
    {
        print_failure("snapshot_restore", "restore verification", error);
        return false;
    }

    if (!run_increment_program(machine, error))
    {
        print_failure("snapshot_restore", "increment program after restore", error);
        return false;
    }

    const CpuRegisters final_registers = machine.read_registers();
    (void)std::printf("perfect6502_debug snapshot_restore PASS: A=%02X PC=%04X half=%llu\n",
                      static_cast<unsigned>(final_registers.a),
                      static_cast<unsigned>(final_registers.pc),
                      static_cast<unsigned long long>(machine.half_cycle()));
    return true;
}

} // namespace perfect6502_debug
