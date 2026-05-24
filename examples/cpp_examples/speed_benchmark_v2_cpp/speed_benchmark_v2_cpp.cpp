#include <qe6502/cpu.hpp>

#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

static const std::uint8_t ROM_6502_FUNCTIONAL_TEST[0x10000] =
#include "../../c_examples/speed_benchmark/6502_functional_test.hex"

namespace {

struct test_case {
    const char* name;
    qe6502::model model;
    const std::uint8_t* rom;
    const char* rom_name;
    std::uint16_t success_address;
    std::uint64_t expected_instructions;
};

struct test_result {
    std::uint64_t ticks;
    std::uint64_t instructions;
    double elapsed_seconds;
    double mhz;
};

constexpr test_case tests[] = {
    {
        "NMOS 6502 functional test",
        qe6502::model::nmos,
        ROM_6502_FUNCTIONAL_TEST,
        "6502_functional_test.hex",
        0x3469u,
        30646176ull,
    },
};

double now_seconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

void print_hex8(std::uint8_t value)
{
    std::printf("0x%02X", static_cast<unsigned>(value));
}

void print_hex16(std::uint16_t value)
{
    std::printf("0x%04X", static_cast<unsigned>(value));
}

void tick_fast(qe6502::cpu& cpu, std::uint8_t memory[0x10000])
{
    const std::uint16_t address = cpu.address();
    const std::uint8_t data = cpu.writing() ? cpu.data() : memory[address];

    if (cpu.writing()) {
        memory[address] = data;
    }

    cpu.step(data);
}

void print_regs(const qe6502::cpu& cpu)
{
    std::printf("PC     = ");
    print_hex16(cpu.pc());
    std::printf("\n");

    std::printf("A      = ");
    print_hex8(cpu.a());
    std::printf("\n");

    std::printf("X      = ");
    print_hex8(cpu.x());
    std::printf("\n");

    std::printf("Y      = ");
    print_hex8(cpu.y());
    std::printf("\n");

    std::printf("S      = ");
    print_hex8(cpu.s());
    std::printf("\n");

    std::printf("P      = ");
    print_hex8(cpu.p());
    std::printf("\n");
}

void prepare_memory(std::uint8_t memory[0x10000], const std::uint8_t* rom)
{
    std::memcpy(memory, rom, 0x10000u);

    memory[0xFFFC] = 0x00u;
    memory[0xFFFD] = 0x04u;
}

bool run_test(const test_case& test, test_result& out_result)
{
    std::array<std::uint8_t, 0x10000> memory{};
    qe6502::cpu cpu{test.model};
    std::uint64_t ticks = 0;
    std::uint64_t instructions = 0;

    std::printf("========================================================================\n");
    std::printf("%s\n", test.name);
    std::printf("========================================================================\n");

    std::printf("ROM:                   %s\n", test.rom_name);

    std::printf("Success address:       ");
    print_hex16(test.success_address);
    std::printf("\n");

    std::printf("Expected instructions: %" PRIu64 "\n", test.expected_instructions);
    std::printf("\n");

    prepare_memory(memory.data(), test.rom);

    cpu.light_reset();

    while (!cpu.instruction_done() && !cpu.halted()) {
        tick_fast(cpu, memory.data());
        ticks++;
    }

    if (cpu.halted()) {
        std::fprintf(stderr, "CPU boot error\n");
        return false;
    }

    const double started_at = now_seconds();

    while (!cpu.halted()) {
        if (cpu.address() == test.success_address) {
            const double elapsed = now_seconds() - started_at;

            if (instructions != test.expected_instructions) {
                std::fprintf(
                    stderr,
                    "Reached success address, but expected %" PRIu64
                    " instructions, got %" PRIu64 "\n",
                    test.expected_instructions,
                    instructions
                    );
                return false;
            }

            out_result.ticks = ticks;
            out_result.instructions = instructions;
            out_result.elapsed_seconds = elapsed;
            out_result.mhz = (static_cast<double>(ticks) / elapsed) / 1000000.0;

            std::printf("PASS\n\n");
            std::printf("Instructions:          %" PRIu64 "\n", instructions);
            std::printf("Ticks:                 %" PRIu64 "\n", ticks);
            std::printf("Elapsed:               %.2f ms\n", elapsed * 1000.0);
            std::printf("Average tick speed:    %.2f MHz\n", out_result.mhz);
            std::printf("\n");
            std::printf("Final registers:\n");
            print_regs(cpu);
            std::printf("\n");

            return true;
        }

        tick_fast(cpu, memory.data());
        ticks++;

        if (cpu.instruction_done()) {
            instructions++;

            if (instructions > test.expected_instructions * 2u) {
                std::fprintf(stderr, "Instruction limit exceeded: %" PRIu64 "\n", instructions);
                return false;
            }
        }
    }

    std::fprintf(stderr, "CPU halted with status: %u\n", static_cast<unsigned>(cpu.status()));
    return false;
}

} // namespace

int main()
{
    test_result results[sizeof(tests) / sizeof(tests[0])]{};
    const double all_started_at = now_seconds();

    for (std::size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        if (!run_test(tests[i], results[i])) {
            return 1;
        }
    }

    const double total_elapsed = now_seconds() - all_started_at;

    std::printf("========================================================================\n");
    std::printf("ALL TESTS PASSED\n");
    std::printf("Total elapsed: %.2f ms\n", total_elapsed * 1000.0);
    std::printf("========================================================================\n\n");

    std::printf("Summary:\n");

    for (std::size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        std::printf(
            "%s: %" PRIu64 " instr, %" PRIu64 " ticks, %.2f MHz\n",
            tests[i].name,
            results[i].instructions,
            results[i].ticks,
            results[i].mhz
            );
    }

    return 0;
}
