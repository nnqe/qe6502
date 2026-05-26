#include "bootstrap.hpp"
#include "perfect_bridge.hpp"

#include <qe6502/cpu.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace perfect6502_debug
{
namespace
{
constexpr std::size_t MemorySize = 65536u;
constexpr std::uint32_t TestRuns = 32u;
constexpr std::uint32_t MaxOpcodeValue = 256u;
constexpr const char* CycleCountCsvName = "singlestep_opcode_cycle_counts.csv";
constexpr std::size_t MaxPrintedLegalOpcodes = 16u;

struct BusOutput
{
    std::uint16_t address = 0u;
    std::uint8_t data = 0u;
    bool write = false;
};

struct InvestigationRun
{
    std::array<std::uint8_t, MemorySize> memory{};
    CpuRegisters registers;
    std::vector<BusOutput> perfect_cycles;
};

struct PerfectSeedBase
{
    PerfectMachine machine;
    PerfectSnapshot snapshot;
    CpuRegisters registers;
};

struct MatchRow
{
    std::uint8_t legal_opcode = 0u;
    std::uint32_t min_bus_prefix = 0u;
};

class XorShift32
{
public:
    explicit XorShift32(std::uint32_t seed) :
        state_(seed == 0u ? 0x6d2b79f5u : seed)
    {
    }

    std::uint32_t next_u32()
    {
        std::uint32_t x = state_;
        x ^= x << 13u;
        x ^= x >> 17u;
        x ^= x << 5u;
        state_ = x;
        return x;
    }

    std::uint8_t next_u8()
    {
        return static_cast<std::uint8_t>(next_u32() & 0xffu);
    }

    std::uint16_t next_u16()
    {
        return static_cast<std::uint16_t>(next_u32() & 0xffffu);
    }

private:
    std::uint32_t state_ = 0u;
};

bool is_nmos_legal_opcode(std::uint8_t opcode)
{
    switch (opcode)
    {
    case 0x00: case 0x01: case 0x05: case 0x06: case 0x08: case 0x09: case 0x0a: case 0x0d: case 0x0e:
    case 0x10: case 0x11: case 0x15: case 0x16: case 0x18: case 0x19: case 0x1d: case 0x1e:
    case 0x20: case 0x21: case 0x24: case 0x25: case 0x26: case 0x28: case 0x29: case 0x2a: case 0x2c: case 0x2d: case 0x2e:
    case 0x30: case 0x31: case 0x35: case 0x36: case 0x38: case 0x39: case 0x3d: case 0x3e:
    case 0x40: case 0x41: case 0x45: case 0x46: case 0x48: case 0x49: case 0x4a: case 0x4c: case 0x4d: case 0x4e:
    case 0x50: case 0x51: case 0x55: case 0x56: case 0x58: case 0x59: case 0x5d: case 0x5e:
    case 0x60: case 0x61: case 0x65: case 0x66: case 0x68: case 0x69: case 0x6a: case 0x6c: case 0x6d: case 0x6e:
    case 0x70: case 0x71: case 0x75: case 0x76: case 0x78: case 0x79: case 0x7d: case 0x7e:
    case 0x81: case 0x84: case 0x85: case 0x86: case 0x88: case 0x8a: case 0x8c: case 0x8d: case 0x8e:
    case 0x90: case 0x91: case 0x94: case 0x95: case 0x96: case 0x98: case 0x99: case 0x9a: case 0x9d:
    case 0xa0: case 0xa1: case 0xa2: case 0xa4: case 0xa5: case 0xa6: case 0xa8: case 0xa9: case 0xaa: case 0xac: case 0xad: case 0xae:
    case 0xb0: case 0xb1: case 0xb4: case 0xb5: case 0xb6: case 0xb8: case 0xb9: case 0xba: case 0xbc: case 0xbd: case 0xbe:
    case 0xc0: case 0xc1: case 0xc4: case 0xc5: case 0xc6: case 0xc8: case 0xc9: case 0xca: case 0xcc: case 0xcd: case 0xce:
    case 0xd0: case 0xd1: case 0xd5: case 0xd6: case 0xd8: case 0xd9: case 0xdd: case 0xde:
    case 0xe0: case 0xe1: case 0xe4: case 0xe5: case 0xe6: case 0xe8: case 0xe9: case 0xea: case 0xec: case 0xed: case 0xee:
    case 0xf0: case 0xf1: case 0xf5: case 0xf6: case 0xf8: case 0xf9: case 0xfd: case 0xfe:
        return true;
    default:
        return false;
    }
}

std::vector<std::uint8_t> make_legal_opcode_list()
{
    std::vector<std::uint8_t> opcodes;
    for (std::uint32_t opcode = 0u; opcode < MaxOpcodeValue; ++opcode)
    {
        const std::uint8_t byte_opcode = static_cast<std::uint8_t>(opcode);
        if (is_nmos_legal_opcode(byte_opcode))
        {
            opcodes.push_back(byte_opcode);
        }
    }

    return opcodes;
}

std::vector<std::uint8_t> make_illegal_opcode_list()
{
    std::vector<std::uint8_t> opcodes;
    for (std::uint32_t opcode = 0u; opcode < MaxOpcodeValue; ++opcode)
    {
        const std::uint8_t byte_opcode = static_cast<std::uint8_t>(opcode);
        if (!is_nmos_legal_opcode(byte_opcode))
        {
            opcodes.push_back(byte_opcode);
        }
    }

    return opcodes;
}

std::uint32_t make_seed(std::uint32_t run_index)
{
    return 0x9e3779b9u ^ (run_index * 0x85ebca6bu) ^ 0xc2b2ae35u;
}

void make_random_memory_and_registers(std::uint32_t run_index,
                                      std::array<std::uint8_t, MemorySize>& memory,
                                      CpuRegisters& registers)
{
    XorShift32 rng(make_seed(run_index));

    for (std::uint8_t& byte : memory)
    {
        byte = rng.next_u8();
    }

    registers.pc = rng.next_u16();
    registers.a = rng.next_u8();
    registers.x = rng.next_u8();
    registers.y = rng.next_u8();
    registers.s = rng.next_u8();
    registers.p = rng.next_u8();
    registers.ir = 0u;
}

void write_memory_image(PerfectMachine& machine, const std::array<std::uint8_t, MemorySize>& memory)
{
    for (std::uint32_t address = 0u; address < MemorySize; ++address)
    {
        machine.write_memory(static_cast<std::uint16_t>(address), memory[address]);
    }
}

void read_memory_image(const PerfectMachine& machine, std::array<std::uint8_t, MemorySize>& memory)
{
    for (std::uint32_t address = 0u; address < MemorySize; ++address)
    {
        memory[address] = machine.read_memory(static_cast<std::uint16_t>(address));
    }
}

BusOutput read_perfect_bus_output(const PerfectMachine& machine)
{
    BusOutput output;
    output.address = machine.read_address_bus();
    output.write = !machine.is_read();
    if (output.write)
    {
        output.data = machine.read_data_bus();
    }
    return output;
}

BusOutput read_qe_bus_output(const qe6502::cpu& cpu)
{
    BusOutput output;
    output.address = cpu.address();
    output.write = cpu.writing();
    if (output.write)
    {
        output.data = cpu.data();
    }
    return output;
}

bool bus_matches(const BusOutput& left, const BusOutput& right)
{
    if (left.address != right.address)
    {
        return false;
    }

    if (left.write != right.write)
    {
        return false;
    }

    if (left.write && (left.data != right.data))
    {
        return false;
    }

    return true;
}

bool build_seed_base(std::uint32_t run_index, PerfectSeedBase& base, std::string& error)
{
    std::array<std::uint8_t, MemorySize> initial_memory{};
    CpuRegisters target_registers;
    make_random_memory_and_registers(run_index, initial_memory, target_registers);

    if (!base.machine.reset(error))
    {
        return false;
    }

    write_memory_image(base.machine, initial_memory);

    if (!bootstrap_to_registers(base.machine,
                                SetupCodeAddress,
                                SetupDataAddress,
                                target_registers,
                                error))
    {
        return false;
    }

    base.registers = base.machine.read_registers();
    base.snapshot = base.machine.create_snapshot(error);
    return base.snapshot.is_valid();
}

bool build_seed_bases(std::array<PerfectSeedBase, TestRuns>& bases, std::string& error)
{
    for (std::uint32_t run_index = 0u; run_index < TestRuns; ++run_index)
    {
        if (!build_seed_base(run_index, bases[run_index], error))
        {
            return false;
        }
    }

    return true;
}

bool build_perfect_run(std::uint8_t illegal_opcode,
                       std::uint32_t illegal_cycle_limit,
                       PerfectSeedBase& base,
                       InvestigationRun& run,
                       std::string& error)
{
    if (!base.machine.restore_snapshot(base.snapshot, error))
    {
        return false;
    }

    run.registers = base.registers;
    base.machine.write_memory(run.registers.pc, illegal_opcode);
    read_memory_image(base.machine, run.memory);

    run.perfect_cycles.clear();
    run.perfect_cycles.reserve(illegal_cycle_limit);

    for (std::uint32_t cycle = 0u; cycle < illegal_cycle_limit; ++cycle)
    {
        base.machine.step_half_cycle();
        base.machine.step_half_cycle();
        run.perfect_cycles.push_back(read_perfect_bus_output(base.machine));
    }

    return true;
}

std::uint32_t count_legal_prefix_match(const InvestigationRun& run, std::uint8_t legal_opcode)
{
    std::array<std::uint8_t, MemorySize> memory = run.memory;
    memory[run.registers.pc] = legal_opcode;

    qe6502::cpu cpu(qe6502::model::nmos);
    cpu.a(run.registers.a);
    cpu.x(run.registers.x);
    cpu.y(run.registers.y);
    cpu.s(run.registers.s);
    cpu.p(run.registers.p);
    cpu.go_to(run.registers.pc);

    std::uint32_t matched = 0u;

    for (std::size_t cycle = 0u; cycle < run.perfect_cycles.size(); ++cycle)
    {
        const std::uint16_t input_address = cpu.address();
        const std::uint8_t input_data = cpu.writing() ? cpu.data() : memory[input_address];

        if (cpu.writing())
        {
            memory[input_address] = input_data;
        }

        cpu.step(input_data);

        const BusOutput qe_output = read_qe_bus_output(cpu);
        if (!bus_matches(qe_output, run.perfect_cycles[cycle]))
        {
            break;
        }

        ++matched;

        if (cpu.fetching() || cpu.trapped())
        {
            break;
        }
    }

    return matched;
}

std::vector<InvestigationRun> build_perfect_runs(std::uint8_t illegal_opcode,
                                                 std::uint32_t illegal_cycle_limit,
                                                 std::array<PerfectSeedBase, TestRuns>& bases,
                                                 std::string& error)
{
    std::vector<InvestigationRun> runs;
    runs.resize(TestRuns);

    for (std::uint32_t run_index = 0u; run_index < TestRuns; ++run_index)
    {
        if (!build_perfect_run(illegal_opcode, illegal_cycle_limit, bases[run_index], runs[run_index], error))
        {
            runs.clear();
            return runs;
        }
    }

    return runs;
}

std::vector<MatchRow> investigate_one_illegal_opcode(std::uint8_t illegal_opcode,
                                                     std::uint32_t illegal_cycle_limit,
                                                     const std::vector<std::uint8_t>& legal_opcodes,
                                                     std::array<PerfectSeedBase, TestRuns>& bases,
                                                     std::string& error)
{
    std::vector<MatchRow> all_rows;
    std::vector<InvestigationRun> runs = build_perfect_runs(illegal_opcode, illegal_cycle_limit, bases, error);
    if (runs.empty())
    {
        return all_rows;
    }

    std::uint32_t best_min_bus_prefix = 0u;

    for (std::uint8_t legal_opcode : legal_opcodes)
    {
        std::uint32_t min_bus_prefix = illegal_cycle_limit;

        for (const InvestigationRun& run : runs)
        {
            const std::uint32_t prefix = count_legal_prefix_match(run, legal_opcode);
            if (prefix < min_bus_prefix)
            {
                min_bus_prefix = prefix;
            }

            if (min_bus_prefix < best_min_bus_prefix)
            {
                break;
            }
        }

        if (min_bus_prefix == 0u)
        {
            continue;
        }

        if (min_bus_prefix > best_min_bus_prefix)
        {
            best_min_bus_prefix = min_bus_prefix;
            all_rows.clear();
        }

        if (min_bus_prefix == best_min_bus_prefix)
        {
            all_rows.push_back(MatchRow{legal_opcode, min_bus_prefix});
        }
    }

    std::sort(all_rows.begin(), all_rows.end(), [](const MatchRow& left, const MatchRow& right) {
        return left.legal_opcode < right.legal_opcode;
    });

    return all_rows;
}

void print_hex_byte(FILE* output, std::uint8_t value)
{
    (void)std::fprintf(output, "%02X", static_cast<unsigned>(value));
}

std::string trim_copy(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return std::string();
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1u);
}

std::vector<std::string> split_csv_line(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    std::istringstream input(line);
    while (std::getline(input, field, ','))
    {
        fields.push_back(trim_copy(field));
    }

    return fields;
}

bool parse_hex_byte(const std::string& text, std::uint8_t& value)
{
    if (text.size() != 2u)
    {
        return false;
    }

    unsigned parsed = 0u;
    if (std::sscanf(text.c_str(), "%x", &parsed) != 1)
    {
        return false;
    }

    if (parsed > 0xffu)
    {
        return false;
    }

    value = static_cast<std::uint8_t>(parsed);
    return true;
}

bool parse_uint32(const std::string& text, std::uint32_t& value)
{
    unsigned long parsed = 0u;
    char tail = '\0';
    if (std::sscanf(text.c_str(), "%lu%c", &parsed, &tail) != 1)
    {
        return false;
    }

    value = static_cast<std::uint32_t>(parsed);
    return true;
}

std::string parent_directory(const char* path)
{
    if (path == nullptr)
    {
        return std::string();
    }

    const std::string text(path);
    const std::size_t slash = text.find_last_of("/\\");
    if (slash == std::string::npos)
    {
        return std::string();
    }

    return text.substr(0u, slash);
}

std::string join_path(const std::string& directory, const char* filename)
{
    if (directory.empty())
    {
        return std::string(filename);
    }

    return directory + "/" + filename;
}

bool read_cycle_count_csv(const std::string& path,
                          std::array<std::uint32_t, MaxOpcodeValue>& max_cycles_by_opcode,
                          std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "cannot open cycle count csv: " + path;
        return false;
    }

    std::string line;
    if (!std::getline(input, line))
    {
        error = "empty cycle count csv: " + path;
        return false;
    }

    while (std::getline(input, line))
    {
        if (trim_copy(line).empty())
        {
            continue;
        }

        const std::vector<std::string> fields = split_csv_line(line);
        if (fields.size() < 5u)
        {
            error = "bad cycle count csv row: " + line;
            return false;
        }

        std::uint8_t opcode = 0u;
        std::uint32_t max_cycles = 0u;
        if (!parse_hex_byte(fields[0], opcode) || !parse_uint32(fields[4], max_cycles) || (max_cycles == 0u))
        {
            error = "bad cycle count csv values: " + line;
            return false;
        }

        max_cycles_by_opcode[opcode] = max_cycles;
    }

    return true;
}

bool load_cycle_count_table(const char* executable_path,
                            std::array<std::uint32_t, MaxOpcodeValue>& max_cycles_by_opcode,
                            std::string& error)
{
    max_cycles_by_opcode.fill(0u);

    const std::string executable_directory = parent_directory(executable_path);
    const std::array<std::string, 3u> candidates = {
        join_path(executable_directory, CycleCountCsvName),
        std::string(CycleCountCsvName),
        std::string("harness/transistor_netlist_test/debugger/") + CycleCountCsvName
    };

    std::string last_error;
    for (const std::string& path : candidates)
    {
        if (read_cycle_count_csv(path, max_cycles_by_opcode, last_error))
        {
            return true;
        }
    }

    error = last_error;
    return false;
}

std::string format_legal_group_for_csv(const std::vector<MatchRow>& rows)
{
    if (rows.empty())
    {
        return std::string();
    }

    std::string text;
    char buffer[8];
    const std::size_t count = std::min(rows.size(), MaxPrintedLegalOpcodes);
    for (std::size_t index = 0u; index < count; ++index)
    {
        if (index != 0u)
        {
            text += ';';
        }

        (void)std::snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned>(rows[index].legal_opcode));
        text += buffer;
    }

    return text;
}

std::uint32_t best_prefix_from_rows(const std::vector<MatchRow>& rows)
{
    if (rows.empty())
    {
        return 0u;
    }

    return rows.front().min_bus_prefix;
}

} // namespace

int run_illegal_opcode_investigator(const char* executable_path)
{
    const std::vector<std::uint8_t> legal_opcodes = make_legal_opcode_list();
    const std::vector<std::uint8_t> illegal_opcodes = make_illegal_opcode_list();

    std::string error;
    std::array<std::uint32_t, MaxOpcodeValue> max_cycles_by_opcode{};
    if (!load_cycle_count_table(executable_path, max_cycles_by_opcode, error))
    {
        (void)std::fprintf(stderr, "investigate-illegal cycle table load failed: %s\n", error.c_str());
        return 1;
    }

    std::array<PerfectSeedBase, TestRuns> bases;
    if (!build_seed_bases(bases, error))
    {
        (void)std::fprintf(stderr, "investigate-illegal setup failed: %s\n", error.c_str());
        return 1;
    }

    (void)std::fputs("illegal,max_cycles,best_min_bus_prefix,legal_group_count,legal_group\n", stdout);

    for (std::uint8_t illegal_opcode : illegal_opcodes)
    {
        const std::uint32_t illegal_cycle_limit = max_cycles_by_opcode[illegal_opcode];
        if (illegal_cycle_limit == 0u)
        {
            (void)std::fprintf(stderr,
                               "investigate-illegal missing cycle limit for opcode %02X\n",
                               static_cast<unsigned>(illegal_opcode));
            return 1;
        }

        error.clear();
        const std::vector<MatchRow> rows = investigate_one_illegal_opcode(illegal_opcode,
                                                                          illegal_cycle_limit,
                                                                          legal_opcodes,
                                                                          bases,
                                                                          error);
        if (!error.empty())
        {
            (void)std::fprintf(stderr,
                               "investigate-illegal failed for opcode %02X: %s\n",
                               static_cast<unsigned>(illegal_opcode),
                               error.c_str());
            return 1;
        }

        print_hex_byte(stdout, illegal_opcode);
        (void)std::fprintf(stdout,
                           ",%u,%u,%u,%s\n",
                           static_cast<unsigned>(illegal_cycle_limit),
                           static_cast<unsigned>(best_prefix_from_rows(rows)),
                           static_cast<unsigned>(rows.size()),
                           format_legal_group_for_csv(rows).c_str());
    }

    return 0;
}

} // namespace perfect6502_debug
