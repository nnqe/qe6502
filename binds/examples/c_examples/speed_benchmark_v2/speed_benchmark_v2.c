// bench_qe6502_v2_native_embedded.c

#include "qe6502.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const uint8_t ROM_6502_FUNCTIONAL_TEST[0x10000] =
#include "../speed_benchmark/6502_functional_test.hex"

static const uint8_t ROM_65C02_EXTENDED_OPCODES_TEST[0x10000] =
#include "../speed_benchmark/65C02_extended_opcodes_test.hex"


typedef struct {
    const char* model_name;
    uint8_t model;
    const uint8_t* rom;
    const char* klaus_name;
    uint16_t success_address;
    uint64_t expected_instructions;
} test_case_t;

typedef struct {
    uint64_t ticks;
    uint64_t instructions;
    double elapsed_seconds;
    double mhz;
} test_result_t;

static const test_case_t TEST_NMOS = {
    "NMOS",
    qe6502_model_nmos,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

static const test_case_t TEST_WDC_EXTENDED = {
    "WDC",
    qe6502_model_wdc,
    ROM_65C02_EXTENDED_OPCODES_TEST,
    "Klaus extended 65C02",
    0x24F1u,
    21986985ull,
};

static const test_case_t TEST_WDC_STANDARD = {
    "WDC",
    qe6502_model_wdc,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

static const test_case_t TEST_RW_EXTENDED = {
    "RW",
    qe6502_model_rw,
    ROM_65C02_EXTENDED_OPCODES_TEST,
    "Klaus extended 65C02",
    0x24F1u,
    21986985ull,
};

static const test_case_t TEST_ST_STANDARD = {
    "ST",
    qe6502_model_st,
    ROM_6502_FUNCTIONAL_TEST,
    "Klaus standard 6502",
    0x3469u,
    30646176ull,
};

static double now_seconds(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static uint8_t tick_is_write(qe6502_tick_t tick) {
    return (uint8_t)((tick.status & qe6502_status_writing) != 0u);
}

static uint8_t tick_is_opcode_fetch(qe6502_tick_t tick) {
    return (uint8_t)((tick.status & qe6502_status_opcode_fetch) != 0u);
}

static uint8_t tick_bus_data(qe6502_tick_t tick, const uint8_t* memory) {
    if (tick_is_write(tick)) {
        return tick.bus;
    }

    return memory[tick.address];
}

static void tick_fast(qe6502_t* cpu, uint8_t memory[0x10000], qe6502_tick_t* tick) {
    const uint8_t data = tick_bus_data(*tick, memory);

    if (tick_is_write(*tick)) {
        memory[tick->address] = data;
    }

    *tick = qe6502_tick(cpu, data);
}

static void prepare_memory(uint8_t memory[0x10000], const uint8_t* rom) {
    memcpy(memory, rom, 0x10000);

    // Same as JS/Python harness: force reset vector to $0400.
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;
}

static void print_result_line(const test_case_t* test, const char* status, double mhz) {
    if (mhz > 0.0) {
        printf("  %-4s | %-21s | %-4s | %8.2f MHz\n", test->model_name, test->klaus_name, status, mhz);
    } else {
        printf("  %-4s | %-21s | %-4s |        - MHz\n", test->model_name, test->klaus_name, status);
    }
}

static int run_test(const test_case_t* test, test_result_t* out_result, uint8_t visible) {
    uint8_t memory[0x10000];
    qe6502_t cpu;
    qe6502_tick_t tick;
    uint64_t ticks = 0;
    uint64_t instructions = 0;
    double started_at;
    double elapsed;

    prepare_memory(memory, test->rom);

    memset(&cpu, 0, sizeof(cpu));
    cpu.model = test->model;

    started_at = now_seconds();

    tick = qe6502_restart(&cpu);

    while (!tick_is_opcode_fetch(tick)) {
        tick_fast(&cpu, memory, &tick);
        ticks++;
    }


    for (;;) {
        if (tick.address == test->success_address) {
            elapsed = now_seconds() - started_at;

            if (instructions != test->expected_instructions) {
                if (visible) {
                    print_result_line(test, "FAIL", 0.0);
                }

                fprintf(
                    stderr,
                    "Reached success address, but expected %" PRIu64
                    " instructions, got %" PRIu64 "\n",
                    test->expected_instructions,
                    instructions
                    );
                return 0;
            }

            out_result->ticks = ticks;
            out_result->instructions = instructions;
            out_result->elapsed_seconds = elapsed;
            out_result->mhz = ((double)ticks / elapsed) / 1000000.0;

            if (visible) {
                print_result_line(test, "PASS", out_result->mhz);
            }

            return 1;
        }

        tick_fast(&cpu, memory, &tick);
        ticks++;

        if (tick_is_opcode_fetch(tick)) {
            instructions++;

            if (instructions > test->expected_instructions * 2u) {
                if (visible) {
                    print_result_line(test, "FAIL", 0.0);
                }

                fprintf(stderr, "Instruction limit exceeded: %" PRIu64 "\n", instructions);
                return 0;
            }
        }
    }

}

static int run_stage(const char* title, const test_case_t* const* tests, size_t test_count) {
    size_t i;

    printf("%s\n", title);

    for (i = 0; i < test_count; i++) {
        test_result_t result;

        memset(&result, 0, sizeof(result));

        if (!run_test(tests[i], &result, 1u)) {
            return 0;
        }
    }

    printf("\n");
    return 1;
}

int main(void) {
    test_result_t cold_result;

    static const test_case_t* const stage1_tests[] = {
        &TEST_NMOS,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_STANDARD,
        &TEST_NMOS,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_STANDARD,
        &TEST_NMOS,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_STANDARD,
    };

    static const test_case_t* const stage2_tests[] = {
        &TEST_NMOS,
        &TEST_NMOS,
        &TEST_NMOS,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_EXTENDED,
        &TEST_WDC_STANDARD,
        &TEST_WDC_STANDARD,
        &TEST_WDC_STANDARD,
    };

    static const test_case_t* const final_tests[] = {
        &TEST_RW_EXTENDED,
        &TEST_ST_STANDARD,
    };

    memset(&cold_result, 0, sizeof(cold_result));

    if (!run_test(&TEST_NMOS, &cold_result, 0u)) {
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

    printf("All speed benchmarks passed.\n");
    return 0;
}
