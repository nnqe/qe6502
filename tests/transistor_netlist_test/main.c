#include <qe6502/qe6502.h>

#include "netlist_helpers.h"
#include "opcode_test.h"
#include "opcode_sweep_test.h"
#include "interrupt_test.h"
#include "setup_test.h"
#include "third_party/perfect6502/perfect6502.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t make_seed(void)
{
    uint32_t seed = (uint32_t)time(NULL);
    seed ^= (uint32_t)clock() * 0x9e3779b9u;

    if (seed == 0u)
    {
        seed = 0x12345678u;
    }

    return seed;
}

static int is_decimal_number(const char* arg)
{
    if ((arg == NULL) || (*arg == '\0'))
    {
        return 0;
    }

    while (*arg != '\0')
    {
        if ((*arg < '0') || (*arg > '9'))
        {
            return 0;
        }

        ++arg;
    }

    return 1;
}

static int is_seed_arg(const char* arg)
{
    if (arg == NULL)
    {
        return 0;
    }

    return ((strncmp(arg, "seed=", 5u) == 0) ||
            (strncmp(arg, "0x", 2u) == 0) ||
            (strncmp(arg, "0X", 2u) == 0) ||
            is_decimal_number(arg));
}

static uint32_t parse_seed(const char* arg)
{
    uint32_t seed;

    if (strncmp(arg, "seed=", 5u) == 0)
    {
        arg += 5u;
    }

    seed = (uint32_t)strtoul(arg, NULL, 0);
    return (seed == 0u) ? 0x12345678u : seed;
}

int main(int argc, char** argv)
{
    static const uint8_t prog_nop[] = {0xeau};
    static const uint8_t prog_lda_imm[] = {0xa9u, 0x42u};
    static const uint8_t prog_ldx_imm[] = {0xa2u, 0x77u};
    static const uint8_t prog_ldy_imm[] = {0xa0u, 0x33u};
    static const uint8_t prog_lda_tax[] = {0xa9u, 0x5au, 0xaau};

    uint8_t qe_mem[65536];
    uint32_t seed = make_seed();
    uint8_t run_sweep = 0u;
    uint8_t include_illegal = 0u;
    uint8_t run_irq_lda = 0u;
    uint32_t sweep_trials = 8u;
    char error[4096];
    int argi;
    size_t i;

    for (argi = 1; argi < argc; ++argi)
    {
        if (strcmp(argv[argi], "sweep_legal") == 0)
        {
            run_sweep = 1u;
            include_illegal = 0u;
        }
        else if (strcmp(argv[argi], "sweep_all") == 0)
        {
            run_sweep = 1u;
            include_illegal = 1u;
        }
        else if (strcmp(argv[argi], "irq_lda") == 0)
        {
            run_irq_lda = 1u;
        }
        else if ((strcmp(argv[argi], "legal") == 0) ||
                 (strcmp(argv[argi], "standard") == 0) ||
                 (strcmp(argv[argi], "mos") == 0))
        {
            /* accepted compatibility arguments */
        }
        else if (strncmp(argv[argi], "seed=", 5u) == 0)
        {
            seed = parse_seed(argv[argi]);
        }
        else if ((run_sweep != 0u) && is_decimal_number(argv[argi]) && (sweep_trials == 8u))
        {
            sweep_trials = (uint32_t)strtoul(argv[argi], NULL, 0);
            if (sweep_trials == 0u)
            {
                sweep_trials = 8u;
            }
        }
        else if (is_seed_arg(argv[argi]))
        {
            seed = parse_seed(argv[argi]);
        }
    }

    printf("transistor-netlist smoke-test seed=%08X\n", seed);

    if (fill_random_memory(qe_mem, memory, &seed) != OK)
    {
        printf("fill_random_memory failed\n");
        return 1;
    }

    {
        qe6502_cpu_t cpu;
        const int setup_rc = run_random_setup_tests(&cpu,
                                                    qe_mem,
                                                    memory,
                                                    &seed,
                                                    16u,
                                                    error,
                                                    sizeof(error));
        if (setup_rc != OK)
        {
            printf("setup test failed: %s\n", error);
            return setup_rc;
        }
    }

    {
        struct program_case_t
        {
            const char* name;
            const uint8_t* data;
            size_t size;
            uint32_t instructions;
        };

        const struct program_case_t programs[] = {
            {"nop", prog_nop, sizeof(prog_nop), 1u},
            {"lda_imm", prog_lda_imm, sizeof(prog_lda_imm), 1u},
            {"ldx_imm", prog_ldx_imm, sizeof(prog_ldx_imm), 1u},
            {"ldy_imm", prog_ldy_imm, sizeof(prog_ldy_imm), 1u},
            {"lda_tax", prog_lda_tax, sizeof(prog_lda_tax), 2u}
        };

        for (i = 0u; i < (sizeof(programs) / sizeof(programs[0])); ++i)
        {
            qe6502_cpu_t cpu;
            const int rc = run_program_test(&cpu,
                                            qe_mem,
                                            memory,
                                            programs[i].data,
                                            programs[i].size,
                                            programs[i].instructions,
                                            &seed,
                                            error,
                                            sizeof(error));
            if (rc != OK)
            {
                printf("program %s failed: %s\n", programs[i].name, error);
                return rc;
            }
        }
    }


    if (run_irq_lda != 0u)
    {
        qe6502_cpu_t cpu;
        const int rc = run_irq_lda_abs_timing_test(&cpu,
                                                   qe_mem,
                                                   memory,
                                                   &seed,
                                                   error,
                                                   sizeof(error));
        if (rc != OK)
        {
            printf("irq lda test failed: %s\n", error);
            return rc;
        }
    }

    if (run_sweep != 0u)
    {
        qe6502_cpu_t cpu;
        const int rc = run_opcode_sweep_test(&cpu,
                                             qe_mem,
                                             memory,
                                             &seed,
                                             sweep_trials,
                                             include_illegal,
                                             error,
                                             sizeof(error));
        if (rc != OK)
        {
            printf("opcode sweep failed: %s\n", error);
            return rc;
        }
    }

    printf("End.\n");
    return 0;
}
