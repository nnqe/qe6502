// bench_qe6502_native_embedded.c

#include <qe6502/qe6502.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TICK_ADDRESS_MASK 0x0000FFFFu
#define TICK_BUS_WRITE    0x00010000u
#define TICK_STARTING     0x00020000u
#define TICK_INSTR_DONE   0x00040000u
#define TICK_NOT_OK       0x00800000u
#define TICK_DATA_SHIFT   24u

static const uint8_t ROM_6502_FUNCTIONAL_TEST[0x10000] =
#include "6502_functional_test.hex"


static const uint8_t ROM_65C02_EXTENDED_OPCODES_TEST[0x10000] =
#include "65C02_extended_opcodes_test.hex"


typedef struct {
    const char* name;
    uint8_t model;
    const uint8_t* rom;
    const char* rom_name;
    uint16_t success_address;
    uint64_t expected_instructions;
} test_case_t;

typedef struct {
    uint16_t address;
    uint8_t is_write;
    uint8_t is_started;
    uint8_t is_instr_done;
    uint8_t ok;
    uint8_t data_out;
} bus_state_t;

typedef struct {
    uint64_t ticks;
    uint64_t instructions;
    double elapsed_seconds;
    double mhz;
} test_result_t;

static const test_case_t TESTS[] = {
    {
        "RW 65C02 functional test",
        QE6502_MODEL_RW,
        ROM_6502_FUNCTIONAL_TEST,
        "6502_functional_test.hex",
        0x3469,
        30646176ull,
    },
    {
        "RW 65C02 extended opcodes test",
        QE6502_MODEL_RW,
        ROM_65C02_EXTENDED_OPCODES_TEST,
        "65C02_extended_opcodes_test.hex",
        0x24F1,
        21986985ull,
    },
    {
        "WDC 65C02 extended opcodes test",
        QE6502_MODEL_WDC,
        ROM_65C02_EXTENDED_OPCODES_TEST,
        "65C02_extended_opcodes_test.hex",
        0x24F1,
        21986985ull,
    },
    {
        "ST 65C02 functional test",
        QE6502_MODEL_ST,
        ROM_6502_FUNCTIONAL_TEST,
        "6502_functional_test.hex",
        0x3469,
        30646176ull,
    },
    {
        "WDC 65C02 functional test",
        QE6502_MODEL_WDC,
        ROM_6502_FUNCTIONAL_TEST,
        "6502_functional_test.hex",
        0x3469,
        30646176ull,
    },
    {
        "MOS 6502 functional test",
        QE6502_MODEL_MOS,
        ROM_6502_FUNCTIONAL_TEST,
        "6502_functional_test.hex",
        0x3469,
        30646176ull,
    },
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

static void print_hex8(uint8_t value) {
    printf("0x%02X", (unsigned)value);
}

static void print_hex16(uint16_t value) {
    printf("0x%04X", (unsigned)value);
}

static void refresh_state_slow(const qe6502_cpu_t* cpu, bus_state_t* state) {
    uint8_t has_data = qe6502_has_data(cpu) != 0;
    uint8_t needs_data = qe6502_needs_data(cpu) != 0;

    state->address = qe6502_address(cpu);
    state->is_write = has_data && !needs_data;
    state->is_started = qe6502_is_started(cpu) != 0;
    state->is_instr_done = qe6502_is_instr_done(cpu) != 0;
    state->ok = qe6502_ok(cpu) != 0;
    state->data_out = state->is_write ? qe6502_read_data(cpu) : 0;
}

static void decode_tick_state(uint32_t packed, bus_state_t* state) {
    state->address = (uint16_t)(packed & TICK_ADDRESS_MASK);
    state->is_write = (packed & TICK_BUS_WRITE) != 0;
    state->is_started = (packed & TICK_STARTING) == 0;
    state->is_instr_done = (packed & TICK_INSTR_DONE) != 0;
    state->ok = (packed & TICK_NOT_OK) == 0;
    state->data_out = (uint8_t)((packed >> TICK_DATA_SHIFT) & 0xFFu);
}

static void tick_fast(qe6502_cpu_t* cpu, uint8_t memory[0x10000], bus_state_t* state) {
    uint8_t data_in = 0;
    uint32_t packed;

    if (state->is_write) {
        memory[state->address] = state->data_out;
    } else {
        data_in = memory[state->address];
    }

    packed = qe6502_cpu_tick_ex(cpu, data_in);
    decode_tick_state(packed, state);
}

static void print_regs(const qe6502_cpu_t* cpu) {
    uint64_t packed = qe6502_dump(cpu);

    uint16_t pc = (uint16_t)(
        (packed & 0xFFu) |
        (((packed >> 8u) & 0xFFu) << 8u)
        );

    uint8_t a = (uint8_t)((packed >> 16u) & 0xFFu);
    uint8_t x = (uint8_t)((packed >> 24u) & 0xFFu);
    uint8_t y = (uint8_t)((packed >> 32u) & 0xFFu);
    uint8_t s = (uint8_t)((packed >> 40u) & 0xFFu);
    uint8_t p = (uint8_t)((packed >> 48u) & 0xFFu);

    printf("PC     = ");
    print_hex16(pc);
    printf("\n");

    printf("A      = ");
    print_hex8(a);
    printf("\n");

    printf("X      = ");
    print_hex8(x);
    printf("\n");

    printf("Y      = ");
    print_hex8(y);
    printf("\n");

    printf("S      = ");
    print_hex8(s);
    printf("\n");

    printf("P      = ");
    print_hex8(p);
    printf("\n");
}

static void prepare_memory(uint8_t memory[0x10000], const uint8_t* rom) {
    memcpy(memory, rom, 0x10000);

    // Same as JS/Python harness: force reset vector to $0400.
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;
}

static int run_test(const test_case_t* test, test_result_t* out_result) {
    uint8_t* memory;
    qe6502_cpu_t cpu;
    bus_state_t state;
    uint64_t ticks = 0;
    uint64_t instructions = 0;
    double started_at;
    double elapsed;

    printf("========================================================================\n");
    printf("%s\n", test->name);
    printf("========================================================================\n");

    printf("ROM:                   %s\n", test->rom_name);

    printf("Success address:       ");
    print_hex16(test->success_address);
    printf("\n");

    printf("Expected instructions: %" PRIu64 "\n", test->expected_instructions);
    printf("\n");

    memory = (uint8_t*)malloc(0x10000u);

    if (!memory) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }

    prepare_memory(memory, test->rom);

    qe6502_cpu_power_on(&cpu, test->model);
    refresh_state_slow(&cpu, &state);

    while (!state.is_started) {
        tick_fast(&cpu, memory, &state);
        ticks++;
    }

    started_at = now_seconds();

    while (state.ok) {
        if (state.address == test->success_address) {
            elapsed = now_seconds() - started_at;

            if (instructions != test->expected_instructions) {
                fprintf(
                    stderr,
                    "Reached success address, but expected %" PRIu64
                    " instructions, got %" PRIu64 "\n",
                    test->expected_instructions,
                    instructions
                    );
                free(memory);
                return 0;
            }

            out_result->ticks = ticks;
            out_result->instructions = instructions;
            out_result->elapsed_seconds = elapsed;
            out_result->mhz = ((double)ticks / elapsed) / 1000000.0;

            printf("PASS\n\n");
            printf("Instructions:          %" PRIu64 "\n", instructions);
            printf("Ticks:                 %" PRIu64 "\n", ticks);
            printf("Elapsed:               %.2f ms\n", elapsed * 1000.0);
            printf("Average tick speed:    %.2f MHz\n", out_result->mhz);
            printf("\n");
            printf("Final registers:\n");
            print_regs(&cpu);
            printf("\n");

            free(memory);
            return 1;
        }

        tick_fast(&cpu, memory, &state);
        ticks++;

        if (state.is_instr_done) {
            instructions++;

            if (instructions > test->expected_instructions * 2u) {
                fprintf(
                    stderr,
                    "Instruction limit exceeded: %" PRIu64 "\n",
                    instructions
                    );
                free(memory);
                return 0;
            }
        }
    }

    fprintf(
        stderr,
        "CPU failed with error code: %u\n",
        (unsigned)qe6502_error_code(&cpu)
        );

    free(memory);
    return 0;
}

int main(int argc, char** argv) {
    const char* only = NULL;
    double all_started_at;
    double total_elapsed;
    test_result_t results[sizeof(TESTS) / sizeof(TESTS[0])];
    const test_case_t* result_tests[sizeof(TESTS) / sizeof(TESTS[0])];
    size_t result_count = 0;
    size_t i;

    for (i = 1; i < (size_t)argc; i++) {
        if (strcmp(argv[i], "--only") == 0 && i + 1 < (size_t)argc) {
            only = argv[++i];
        } else {
            fprintf(stderr, "Usage: %s [--only TEXT]\n", argv[0]);
            return 2;
        }
    }

    all_started_at = now_seconds();

    for (i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); i++) {
        if (only && !strstr(TESTS[i].name, only)) {
            continue;
        }

        if (!run_test(&TESTS[i], &results[result_count])) {
            return 1;
        }

        result_tests[result_count] = &TESTS[i];
        result_count++;
    }

    if (result_count == 0) {
        fprintf(stderr, "No tests matched\n");
        return 1;
    }

    total_elapsed = now_seconds() - all_started_at;

    printf("========================================================================\n");
    printf("ALL TESTS PASSED\n");
    printf("Total elapsed: %.2f ms\n", total_elapsed * 1000.0);
    printf("========================================================================\n\n");

    printf("Summary:\n");

    for (i = 0; i < result_count; i++) {
        printf(
            "%s: %" PRIu64 " instr, %" PRIu64 " ticks, %.2f MHz\n",
            result_tests[i]->name,
            results[i].instructions,
            results[i].ticks,
            results[i].mhz
            );
    }

    return 0;
}