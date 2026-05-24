// bench_qe6502_v2_native_embedded.c

#include "qe6502.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const uint8_t ROM_6502_FUNCTIONAL_TEST[0x10000] =
#include "../speed_benchmark/6502_functional_test.hex"


typedef struct {
    const char* name;
    uint8_t model;
    const uint8_t* rom;
    const char* rom_name;
    uint16_t success_address;
    uint64_t expected_instructions;
} test_case_t;

typedef struct {
    uint64_t ticks;
    uint64_t instructions;
    double elapsed_seconds;
    double mhz;
} test_result_t;

static const test_case_t TESTS[] = {
    {
        "NMOS 6502 functional test",
        qe6502_model_nmos,
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

static uint8_t tick_is_ok(qe6502_tick_t tick) {
    return (uint8_t)((tick.status & qe6502_status_trapped) == 0u);
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

static void print_regs(const qe6502_t* cpu) {
    printf("PC     = ");
    print_hex16(cpu->PC);
    printf("\n");

    printf("A      = ");
    print_hex8(cpu->A);
    printf("\n");

    printf("X      = ");
    print_hex8(cpu->X);
    printf("\n");

    printf("Y      = ");
    print_hex8(cpu->Y);
    printf("\n");

    printf("S      = ");
    print_hex8(cpu->S);
    printf("\n");

    printf("P      = ");
    print_hex8(cpu->P);
    printf("\n");
}

static void prepare_memory(uint8_t memory[0x10000], const uint8_t* rom) {
    memcpy(memory, rom, 0x10000);

    // Same as JS/Python harness: force reset vector to $0400.
    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;
}

static int run_nes_decimal_smoke_test(void) {
    enum {
        start_address = 0x0200u,
        result_adc_address = 0x0300u,
        result_sbc_address = 0x0301u,
        expected_instructions = 8u
    };

    uint8_t memory[0x10000] = {0};
    qe6502_t cpu;
    qe6502_tick_t tick;
    uint32_t instructions = 0;
    uint64_t ticks = 0;

    printf("========================================================================\n");
    printf("NES decimal semantics smoke test\n");
    printf("========================================================================\n");
    printf("ROM:                   embedded decimal-mode smoke program\n");
    printf("Expected behavior:     ADC/SBC ignore decimal mode on NES\n\n");

    memory[start_address + 0u] = 0xF8u; /* SED */
    memory[start_address + 1u] = 0xA9u; /* LDA #$15 */
    memory[start_address + 2u] = 0x15u;
    memory[start_address + 3u] = 0x18u; /* CLC */
    memory[start_address + 4u] = 0x69u; /* ADC #$27: NES binary result is $3C, NMOS decimal would be $42. */
    memory[start_address + 5u] = 0x27u;
    memory[start_address + 6u] = 0x8Du; /* STA $0300 */
    memory[start_address + 7u] = (uint8_t)(result_adc_address & 0xffu);
    memory[start_address + 8u] = (uint8_t)(result_adc_address >> 8u);
    memory[start_address + 9u] = 0x38u; /* SEC */
    memory[start_address + 10u] = 0xE9u; /* SBC #$05: NES binary result is $37. */
    memory[start_address + 11u] = 0x05u;
    memory[start_address + 12u] = 0x8Du; /* STA $0301 */
    memory[start_address + 13u] = (uint8_t)(result_sbc_address & 0xffu);
    memory[start_address + 14u] = (uint8_t)(result_sbc_address >> 8u);

    memset(&cpu, 0, sizeof(cpu));
    cpu.model = qe6502_model_nes;

    tick = qe6502_goto(&cpu, (uint16_t)start_address);

    while (tick_is_ok(tick) && instructions < expected_instructions) {
        tick_fast(&cpu, memory, &tick);
        ticks++;

        if (tick_is_opcode_fetch(tick)) {
            instructions++;
        }
    }

    if (!tick_is_ok(tick)) {
        fprintf(stderr, "NES decimal smoke test trapped with status: %u\n", (unsigned)tick.status);
        return 0;
    }

    if (instructions != expected_instructions) {
        fprintf(stderr, "NES decimal smoke test instruction count mismatch: %u\n", (unsigned)instructions);
        return 0;
    }

    if (memory[result_adc_address] != 0x3Cu || memory[result_sbc_address] != 0x37u) {
        fprintf(
            stderr,
            "NES decimal smoke test failed: ADC result=0x%02X, SBC result=0x%02X\n",
            (unsigned)memory[result_adc_address],
            (unsigned)memory[result_sbc_address]
            );
        return 0;
    }

    printf("PASS\n\n");
    printf("Instructions:          %u\n", (unsigned)instructions);
    printf("Ticks:                 %" PRIu64 "\n", ticks);
    printf("ADC result:            0x%02X\n", (unsigned)memory[result_adc_address]);
    printf("SBC result:            0x%02X\n", (unsigned)memory[result_sbc_address]);
    printf("\n");

    return 1;
}

static int run_test(const test_case_t* test, test_result_t* out_result) {
    uint8_t* memory;
    qe6502_t cpu;
    qe6502_tick_t tick;
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

    memset(&cpu, 0, sizeof(cpu));
    cpu.model = test->model;

    tick = qe6502_reset(&cpu);

    while (!tick_is_opcode_fetch(tick) && tick_is_ok(tick)) {
        tick_fast(&cpu, memory, &tick);
        ticks++;
    }

    if (!tick_is_ok(tick)) {
        fprintf(stderr, "CPU boot error\n");
        free(memory);
        return 0;
    }

    started_at = now_seconds();

    while (tick_is_ok(tick)) {
        if (tick.address == test->success_address) {
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

        tick_fast(&cpu, memory, &tick);
        ticks++;

        if (tick_is_opcode_fetch(tick)) {
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

    fprintf(stderr, "CPU trapped with status: %u\n", (unsigned)tick.status);

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
    uint8_t extra_test_ran = 0;
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

    if (!only || strstr("NES decimal semantics smoke test", only)) {
        if (!run_nes_decimal_smoke_test()) {
            return 1;
        }

        extra_test_ran = 1u;
    }

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

    if (result_count == 0 && extra_test_ran == 0u) {
        fprintf(stderr, "No tests matched\n");
        return 1;
    }

    total_elapsed = now_seconds() - all_started_at;

    printf("========================================================================\n");
    printf("ALL TESTS PASSED\n");
    printf("Total elapsed: %.2f ms\n", total_elapsed * 1000.0);
    printf("========================================================================\n\n");

    printf("Summary:\n");

    if (extra_test_ran != 0u) {
        printf("NES decimal semantics smoke test: passed\n");
    }

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
