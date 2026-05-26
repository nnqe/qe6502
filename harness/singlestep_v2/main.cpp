#include <qe6502/cpu.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <string>

namespace {

struct options {
    qe6502::model model = qe6502::model::nmos;
    std::string path;
    int opcode = -1;
    bool all = false;
    bool compare_cycles = true;
    std::uint64_t max_cases = 0;
};

struct counters {
    std::uint64_t files_run = 0;
    std::uint64_t files_skipped = 0;
    std::uint64_t cases_run = 0;
    std::uint64_t cases_failed = 0;
};

void print_usage(const char* argv0)
{
    std::fprintf(stderr,
        "usage: %s --model nmos|nes|wdc|rw|st --path <ProcessorTests dir> (--opcode xx|--all) [--max-cases n] [--no-cycles]\n",
        argv0);
}

bool parse_hex_byte(const std::string& text, int& out)
{
    if (text.size() != 2u) {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 16);
    if (end == nullptr || *end != '\0' || value < 0 || value > 0xff) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

bool parse_args(int argc, char** argv, options& out)
{
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--model" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value == "nmos" || value == "6502") {
                out.model = qe6502::model::nmos;
            } else if (value == "nes" || value == "nes6502") {
                out.model = qe6502::model::nes;
            } else if (value == "wdc" || value == "wdc65c02" || value == "65c02") {
                out.model = qe6502::model::wdc;
            } else if (value == "rw" || value == "rockwell" || value == "rockwell65c02") {
                out.model = qe6502::model::rw;
            } else if (value == "st" || value == "synertek" || value == "synertek65c02") {
                out.model = qe6502::model::st;
            } else {
                std::fprintf(stderr, "unknown model: %s\n", value.c_str());
                return false;
            }
        } else if (arg == "--path" && i + 1 < argc) {
            out.path = argv[++i];
        } else if (arg == "--opcode" && i + 1 < argc) {
            int opcode = -1;
            if (!parse_hex_byte(argv[++i], opcode)) {
                std::fprintf(stderr, "invalid opcode: %s\n", argv[i]);
                return false;
            }
            out.opcode = opcode;
        } else if (arg == "--all") {
            out.all = true;
        } else if (arg == "--no-cycles") {
            out.compare_cycles = false;
        } else if (arg == "--max-cases" && i + 1 < argc) {
            char* end = nullptr;
            const unsigned long long value = std::strtoull(argv[++i], &end, 10);
            if (end == nullptr || *end != '\0') {
                std::fprintf(stderr, "invalid max-cases: %s\n", argv[i]);
                return false;
            }
            out.max_cases = static_cast<std::uint64_t>(value);
        } else {
            std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
            return false;
        }
    }

    if (out.path.empty()) {
        std::fprintf(stderr, "missing --path\n");
        return false;
    }
    if ((out.opcode >= 0 && out.all) || (out.opcode < 0 && !out.all)) {
        std::fprintf(stderr, "use exactly one of --opcode or --all\n");
        return false;
    }
    return true;
}

bool is_documented_6502_opcode(std::uint8_t opcode)
{
    switch (opcode) {
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

bool is_nmos6502_testable_illegal_opcode(std::uint8_t opcode)
{
    switch (opcode) {
    case 0x02: case 0x03: case 0x04: case 0x07: case 0x0b: case 0x0c: case 0x0f:
    case 0x12: case 0x13: case 0x14: case 0x17: case 0x1a: case 0x1b: case 0x1f:
    case 0x22: case 0x23: case 0x27: case 0x2b: case 0x2f:
    case 0x32: case 0x33: case 0x34: case 0x37: case 0x3a: case 0x3b: case 0x3f:
    case 0x42: case 0x43: case 0x44: case 0x47: case 0x4b: case 0x4f:
    case 0x52: case 0x53: case 0x54: case 0x57: case 0x5a: case 0x5b: case 0x5f:
    case 0x62: case 0x63: case 0x64: case 0x67: case 0x6b: case 0x6f:
    case 0x72: case 0x73: case 0x74: case 0x77: case 0x7a: case 0x7b: case 0x7f:
    case 0x83: case 0x87: case 0x8b: case 0x8f: case 0x92:
    case 0x93: case 0x97: case 0x9b: case 0x9c: case 0x9e: case 0x9f:
    case 0xa3: case 0xa7: case 0xab: case 0xaf:
    case 0xb2: case 0xb3: case 0xb7: case 0xbf:
    case 0xc3: case 0xc7: case 0xcb: case 0xcf:
    case 0xd2: case 0xd3: case 0xd4: case 0xd7: case 0xda: case 0xdb: case 0xdf:
    case 0xe3: case 0xe7: case 0xeb: case 0xef:
    case 0xf2: case 0xf3: case 0xf4: case 0xf7: case 0xfa: case 0xfb: case 0xff:
        return true;
    default:
        return false;
    }
}

bool is_wdc65c02_opcode(std::uint8_t opcode)
{
    if (is_documented_6502_opcode(opcode)) {
        return true;
    }

    switch (opcode) {
    /* WDC 65C02 documented opcodes. */
    case 0x04: case 0x0c: case 0x12: case 0x14: case 0x1a: case 0x1c:
    case 0x32: case 0x34: case 0x3a: case 0x3c:
    case 0x52: case 0x5a:
    case 0x64: case 0x72: case 0x74: case 0x7a: case 0x7c:
    case 0x80: case 0x89: case 0x92: case 0x9c: case 0x9e:
    case 0xb2: case 0xd2: case 0xda: case 0xf2: case 0xfa:
    /* RMB/SMB. */
    case 0x07: case 0x17: case 0x27: case 0x37: case 0x47: case 0x57: case 0x67: case 0x77:
    case 0x87: case 0x97: case 0xa7: case 0xb7: case 0xc7: case 0xd7: case 0xe7: case 0xf7:
    /* BBR/BBS. */
    case 0x0f: case 0x1f: case 0x2f: case 0x3f: case 0x4f: case 0x5f: case 0x6f: case 0x7f:
    case 0x8f: case 0x9f: case 0xaf: case 0xbf: case 0xcf: case 0xdf: case 0xef: case 0xff:
    /* WDC legal NOP slots, excluding WAI/STP. */
    case 0x02: case 0x03: case 0x0b: case 0x13: case 0x1b:
    case 0x22: case 0x23: case 0x2b: case 0x33: case 0x3b:
    case 0x42: case 0x43: case 0x44: case 0x4b: case 0x53: case 0x54: case 0x5b: case 0x5c:
    case 0x62: case 0x63: case 0x6b: case 0x73: case 0x7b:
    case 0x82: case 0x83: case 0x8b: case 0x93: case 0x9b:
    case 0xa3: case 0xab: case 0xb3: case 0xbb:
    case 0xc2: case 0xc3:
    case 0xd3: case 0xd4: case 0xdc:
    case 0xe2: case 0xe3: case 0xeb: case 0xf3: case 0xf4: case 0xfb: case 0xfc:
        return true;
    default:
        return false;
    }
}

bool is_rockwell65c02_opcode(std::uint8_t opcode)
{
    if (is_wdc65c02_opcode(opcode)) {
        return true;
    }

    switch (opcode) {
    case 0xcb:
    case 0xdb:
        return true;
    default:
        return false;
    }
}

bool is_synertek65c02_opcode(std::uint8_t opcode)
{
    return is_rockwell65c02_opcode(opcode);
}

bool is_testable_opcode(qe6502::model model, std::uint8_t opcode)
{
    switch (model) {
    case qe6502::model::nmos:
    case qe6502::model::nes:
        return is_documented_6502_opcode(opcode) || is_nmos6502_testable_illegal_opcode(opcode);
    case qe6502::model::wdc:
        return is_wdc65c02_opcode(opcode);
    case qe6502::model::rw:
        return is_rockwell65c02_opcode(opcode);
    case qe6502::model::st:
        return is_synertek65c02_opcode(opcode);
    }
    return false;
}

std::string opcode_filename(std::uint8_t opcode)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.push_back(digits[(opcode >> 4u) & 0x0fu]);
    out.push_back(digits[opcode & 0x0fu]);
    out += ".json";
    return out;
}

std::string join_path(const std::string& dir, const std::string& file)
{
    if (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
        return dir + file;
    }
    return dir + "/" + file;
}

std::uint8_t json_u8(const nlohmann::json& value)
{
    return static_cast<std::uint8_t>(value.get<std::uint32_t>() & 0xffu);
}

std::uint16_t json_u16(const nlohmann::json& value)
{
    return static_cast<std::uint16_t>(value.get<std::uint32_t>() & 0xffffu);
}

void load_ram(std::array<std::uint8_t, 0x10000>& memory, const nlohmann::json& ram)
{
    for (const auto& entry : ram) {
        memory[json_u16(entry.at(0u))] = json_u8(entry.at(1u));
    }
}

void set_initial_state(qe6502::cpu& cpu, const nlohmann::json& initial)
{
    cpu.pc(json_u16(initial.at("pc")));
    cpu.s(json_u8(initial.at("s")));
    cpu.a(json_u8(initial.at("a")));
    cpu.x(json_u8(initial.at("x")));
    cpu.y(json_u8(initial.at("y")));
    cpu.p(json_u8(initial.at("p")));
}

bool compare_u8(const char* name, std::uint8_t actual, std::uint8_t expected)
{
    if (actual != expected) {
        std::fprintf(stderr, "%s mismatch: got 0x%02X expected 0x%02X\n", name, static_cast<unsigned>(actual), static_cast<unsigned>(expected));
        return false;
    }
    return true;
}

bool compare_u16(const char* name, std::uint16_t actual, std::uint16_t expected)
{
    if (actual != expected) {
        std::fprintf(stderr, "%s mismatch: got 0x%04X expected 0x%04X\n", name, static_cast<unsigned>(actual), static_cast<unsigned>(expected));
        return false;
    }
    return true;
}

bool compare_final_state(const qe6502::cpu& cpu, const std::array<std::uint8_t, 0x10000>& memory, const nlohmann::json& final)
{
    bool ok = true;
    ok = compare_u16("PC", cpu.pc(), json_u16(final.at("pc"))) && ok;
    ok = compare_u8("S", cpu.s(), json_u8(final.at("s"))) && ok;
    ok = compare_u8("A", cpu.a(), json_u8(final.at("a"))) && ok;
    ok = compare_u8("X", cpu.x(), json_u8(final.at("x"))) && ok;
    ok = compare_u8("Y", cpu.y(), json_u8(final.at("y"))) && ok;
    ok = compare_u8("P", cpu.p(), json_u8(final.at("p"))) && ok;

    for (const auto& entry : final.at("ram")) {
        const std::uint16_t address = json_u16(entry.at(0u));
        const std::uint8_t expected = json_u8(entry.at(1u));
        if (memory[address] != expected) {
            std::fprintf(stderr, "RAM[0x%04X] mismatch: got 0x%02X expected 0x%02X\n", static_cast<unsigned>(address), static_cast<unsigned>(memory[address]), static_cast<unsigned>(expected));
            ok = false;
        }
    }
    return ok;
}

bool run_case(const nlohmann::json& test_case, qe6502::model model, bool compare_cycles)
{
    std::array<std::uint8_t, 0x10000> memory{};
    qe6502::cpu cpu{model};
    const nlohmann::json& initial = test_case.at("initial");
    const nlohmann::json& final = test_case.at("final");
    const nlohmann::json& cycles = test_case.at("cycles");
    const std::string name = test_case.contains("name") ? test_case.at("name").get<std::string>() : std::string{"<unnamed>"};

    load_ram(memory, initial.at("ram"));
    set_initial_state(cpu, initial);
    cpu.go_to(cpu.pc());

    std::size_t cycle_index = 0u;
    const std::size_t cycles_to_run = compare_cycles ? cycles.size() : static_cast<std::size_t>(0u);
    while (compare_cycles ? (cycle_index < cycles_to_run) : cpu.tick_is_ok()) {
        if (compare_cycles) {
            const nlohmann::json& expected_cycle = cycles.at(cycle_index);
            const std::uint16_t expected_address = json_u16(expected_cycle.at(0u));
            const std::uint8_t expected_data = json_u8(expected_cycle.at(1u));
            const std::string expected_rw = expected_cycle.at(2u).get<std::string>();
            const bool expected_write = expected_rw == "write";

            if (cpu.address() != expected_address) {
                std::fprintf(stderr, "%s: cycle %zu address mismatch: got 0x%04X expected 0x%04X\n", name.c_str(), cycle_index, static_cast<unsigned>(cpu.address()), static_cast<unsigned>(expected_address));
                return false;
            }
            if (cpu.writing() != expected_write) {
                std::fprintf(stderr, "%s: cycle %zu rw mismatch: got %s expected %s\n", name.c_str(), cycle_index, cpu.writing() ? "write" : "read", expected_rw.c_str());
                return false;
            }
            const std::uint8_t actual_data = cpu.writing() ? cpu.data() : memory[cpu.address()];
            if (actual_data != expected_data) {
                std::fprintf(stderr, "%s: cycle %zu data mismatch: got 0x%02X expected 0x%02X\n", name.c_str(), cycle_index, static_cast<unsigned>(actual_data), static_cast<unsigned>(expected_data));
                return false;
            }
        }

        const std::uint8_t bus = cpu.writing() ? cpu.data() : memory[cpu.address()];
        if (cpu.writing()) {
            memory[cpu.address()] = bus;
        }
        cpu.step(bus);
        cycle_index++;

        if (!compare_cycles && cpu.fetching()) {
            break;
        }
    }

    if (!compare_cycles && !cpu.tick_is_ok()) {
        std::fprintf(stderr, "%s: CPU tick is not OK\n", name.c_str());
        return false;
    }
    if (!compare_final_state(cpu, memory, final)) {
        std::fprintf(stderr, "%s: final state mismatch\n", name.c_str());
        return false;
    }
    return true;
}

bool run_file(const options& opts, std::uint8_t opcode, counters& counts)
{
    if (!is_testable_opcode(opts.model, opcode)) {
        counts.files_skipped++;
        std::printf("SKIP %02X unsupported for selected model\n", static_cast<unsigned>(opcode));
        return true;
    }

    const std::string filename = join_path(opts.path, opcode_filename(opcode));
    std::ifstream in{filename};
    if (!in) {
        std::fprintf(stderr, "failed to open %s\n", filename.c_str());
        return false;
    }

    const nlohmann::json tests = nlohmann::json::parse(in);
    if (!tests.is_array()) {
        std::fprintf(stderr, "%s: root JSON value is not an array\n", filename.c_str());
        return false;
    }

    std::uint64_t local_cases = 0;
    for (const auto& test_case : tests) {
        if (opts.max_cases != 0u && local_cases >= opts.max_cases) {
            break;
        }
        if (!run_case(test_case, opts.model, opts.compare_cycles)) {
            counts.cases_failed++;
            std::fprintf(stderr,
                "FAIL opcode %02X case #%llu\n",
                static_cast<unsigned>(opcode),
                static_cast<unsigned long long>(local_cases));
            return false;
        }
        local_cases++;
        counts.cases_run++;
    }

    counts.files_run++;
    std::printf(
        "PASS %02X (%llu cases)\n",
        static_cast<unsigned>(opcode),
        static_cast<unsigned long long>(local_cases));
    return true;
}

bool run_requested(const options& opts, counters& counts)
{
    if (opts.opcode >= 0) {
        return run_file(opts, static_cast<std::uint8_t>(opts.opcode), counts);
    }

    for (unsigned opcode = 0u; opcode <= 0xffu; opcode++) {
        if (!run_file(opts, static_cast<std::uint8_t>(opcode), counts)) {
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    options opts;
    if (!parse_args(argc, argv, opts)) {
        print_usage(argv[0]);
        return 2;
    }

    counters counts;
    try {
        if (!run_requested(opts, counts)) {
            return 1;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    std::printf(
        "SUMMARY files=%llu skipped=%llu cases=%llu failed=%llu\n",
        static_cast<unsigned long long>(counts.files_run),
        static_cast<unsigned long long>(counts.files_skipped),
        static_cast<unsigned long long>(counts.cases_run),
        static_cast<unsigned long long>(counts.cases_failed));

    return counts.cases_failed == 0u ? 0 : 1;
}
