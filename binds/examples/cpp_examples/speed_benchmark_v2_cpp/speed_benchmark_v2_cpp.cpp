#include <qe6502/cpu.hpp>

#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

static const std::uint8_t ROM_6502_FUNCTIONAL_TEST[0x10000] =
#include "../../c_examples/speed_benchmark/6502_functional_test.hex"

static const std::uint8_t ROM_65C02_EXTENDED_OPCODES_TEST[0x10000] =
#include "../../c_examples/speed_benchmark/65C02_extended_opcodes_test.hex"

namespace {

struct test_case {
    const char* model_name;
    qe6502::model model;
    const std::uint8_t* rom;
    const char* klaus_name;
    std::uint16_t success_address;
    std::uint64_t expected_instructions;
};

struct test_result {
    std::uint64_t ticks;
    std::uint64_t instructions;
    double elapsed_seconds;
    double mhz;
};

constexpr test_case test_nmos{
    "NMOS",
    qe6502::model::nmos,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

constexpr test_case test_wdc_extended{
    "WDC",
    qe6502::model::wdc,
    ROM_65C02_EXTENDED_OPCODES_TEST,
    "Klaus extended 65C02",
    0x24F1u,
    21986985ull,
};

constexpr test_case test_wdc_standard{
    "WDC",
    qe6502::model::wdc,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

constexpr test_case test_rw_extended{
    "RW",
    qe6502::model::rw,
    ROM_65C02_EXTENDED_OPCODES_TEST,
    "Klaus extended 65C02",
    0x24F1u,
    21986985ull,
};

constexpr test_case test_st_standard{
    "ST",
    qe6502::model::st,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

double now_seconds()
{
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
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

void prepare_memory(std::uint8_t memory[0x10000], const std::uint8_t* rom)
{
    std::memcpy(memory, rom, 0x10000u);

    memory[0xFFFC] = 0x00u;
    memory[0xFFFD] = 0x04u;
}

void print_result_line(const test_case& test, const char* status, double mhz)
{
    if (mhz > 0.0) {
        std::printf("  %-4s | %-21s | %-4s | %8.2f MHz\n", test.model_name, test.klaus_name, status, mhz);
    } else {
        std::printf("  %-4s | %-21s | %-4s |        - MHz\n", test.model_name, test.klaus_name, status);
    }
}

bool run_test(const test_case& test, test_result& out_result, bool visible)
{
    std::array<std::uint8_t, 0x10000> memory{};
    qe6502::cpu cpu{test.model};
    std::uint64_t ticks = 0;
    std::uint64_t instructions = 0;

    prepare_memory(memory.data(), test.rom);

    const double started_at = now_seconds();

    cpu.restart();

    while (!cpu.fetching()) {
        tick_fast(cpu, memory.data());
        ticks++;
    }


    for (;;) {
        if (cpu.address() == test.success_address) {
            const double elapsed = now_seconds() - started_at;

            if (instructions != test.expected_instructions) {
                if (visible) {
                    print_result_line(test, "FAIL", 0.0);
                }

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

            if (visible) {
                print_result_line(test, "PASS", out_result.mhz);
            }

            return true;
        }

        tick_fast(cpu, memory.data());
        ticks++;

        if (cpu.fetching()) {
            instructions++;

            if (instructions > test.expected_instructions * 2u) {
                if (visible) {
                    print_result_line(test, "FAIL", 0.0);
                }

                std::fprintf(stderr, "Instruction limit exceeded: %" PRIu64 "\n", instructions);
                return false;
            }
        }
    }

}

bool run_stage(const char* title, const test_case* const* tests, std::size_t test_count)
{
    std::printf("%s\n", title);

    for (std::size_t i = 0; i < test_count; i++) {
        test_result result{};

        if (!run_test(*tests[i], result, true)) {
            return false;
        }
    }

    std::printf("\n");
    return true;
}

} // namespace

int main()
{
    static constexpr const test_case* stage1_tests[] = {
        &test_nmos,
        &test_wdc_extended,
        &test_wdc_standard,
        &test_nmos,
        &test_wdc_extended,
        &test_wdc_standard,
        &test_nmos,
        &test_wdc_extended,
        &test_wdc_standard,
    };

    static constexpr const test_case* stage2_tests[] = {
        &test_nmos,
        &test_nmos,
        &test_nmos,
        &test_wdc_extended,
        &test_wdc_extended,
        &test_wdc_extended,
        &test_wdc_standard,
        &test_wdc_standard,
        &test_wdc_standard,
    };

    static constexpr const test_case* final_tests[] = {
        &test_rw_extended,
        &test_st_standard,
    };

    test_result cold_result{};

    if (!run_test(test_nmos, cold_result, false)) {
        return 1;
    }

    if (!run_stage("Stage 1: interleaved model benchmarks", stage1_tests, sizeof(stage1_tests) / sizeof(stage1_tests[0]))) {
        return 1;
    }

    if (!run_stage("Stage 2: grouped model benchmarks", stage2_tests, sizeof(stage2_tests) / sizeof(stage2_tests[0]))) {
        return 1;
    }

    if (!run_stage("Final: additional CMOS model benchmarks", final_tests, sizeof(final_tests) / sizeof(final_tests[0]))) {
        return 1;
    }

    std::printf("All speed benchmarks passed.\n");
    return 0;
}
