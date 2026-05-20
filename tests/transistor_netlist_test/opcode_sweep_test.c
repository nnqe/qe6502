#include "opcode_sweep_test.h"

#include "netlist_helpers.h"
#include "opcode_test.h"

#include <stdio.h>
#include <string.h>

#define MEMORY_REFRESH_PERIOD 256u

static int is_illegal_or_non_mos(const qe6502_opcode_meta_t* meta)
{
    if (meta == NULL)
    {
        return 1;
    }

    if ((meta->name != NULL) && (strstr(meta->name, "ILL") != NULL))
    {
        return 1;
    }

    if (meta->is_cmos_extension != 0u)
    {
        return 1;
    }

    return 0;
}

int run_opcode_sweep_test(qe6502_cpu_t* cpu,
                          uint8_t* qe_mem,
                          uint8_t* perfect_mem,
                          uint32_t* seed,
                          uint32_t trials_per_opcode,
                          uint8_t include_illegal,
                          char* error,
                          size_t error_size)
{
    uint32_t opcode;
    uint32_t tested = 0u;
    uint32_t skipped = 0u;
    uint32_t total = 0u;
    int rc;

    if ((cpu == NULL) ||
        (qe_mem == NULL) ||
        (perfect_mem == NULL) ||
        (seed == NULL) ||
        (trials_per_opcode == 0u))
    {
        (void)snprintf(error, error_size, "invalid run_opcode_sweep_test argument");
        return ERR_ARG;
    }

    for (opcode = 0u; opcode < 256u; ++opcode)
    {
        const qe6502_opcode_meta_t* meta =
            qe6502_opcode_meta((uint8_t)opcode, (uint8_t)QE6502_MODEL_MOS);
        const char* name = (meta != NULL && meta->name != NULL) ? meta->name : "?";
        uint32_t trial;

        if ((include_illegal == 0u) && is_illegal_or_non_mos(meta))
        {
            ++skipped;
            continue;
        }

        for (trial = 0u; trial < trials_per_opcode; ++trial)
        {
            const uint32_t seed_before = *seed;

            if ((total % MEMORY_REFRESH_PERIOD) == 0u)
            {
                rc = fill_random_memory(qe_mem, perfect_mem, seed);
                if (rc != OK)
                {
                    (void)snprintf(error,
                                   error_size,
                                   "opcode sweep memory randomize failed: opcode=%02X name=%s trial=%u total=%u",
                                   (unsigned)opcode,
                                   name,
                                   (unsigned)trial,
                                   (unsigned)total);
                    return rc;
                }
            }

            rc = run_opcode_test_ex(cpu,
                                    qe_mem,
                                    perfect_mem,
                                    (uint8_t)opcode,
                                    (uint8_t)(include_illegal == 0u),
                                    seed,
                                    error,
                                    error_size);
            if (rc != OK)
            {
                char detail[2048];

                (void)snprintf(detail,
                               sizeof(detail),
                               "opcode sweep failed: opcode=%02X name=%s trial=%u total=%u "
                               "seed_before=%08X seed_after=%08X include_illegal=%u: %s",
                               (unsigned)opcode,
                               name,
                               (unsigned)trial,
                               (unsigned)total,
                               seed_before,
                               *seed,
                               (unsigned)include_illegal,
                               error);
                (void)snprintf(error, error_size, "%s", detail);
                return rc;
            }

            ++tested;
            ++total;
        }
    }

    printf("opcode sweep done: tested=%u skipped=%u trials_per_opcode=%u include_illegal=%u seed=%08X\n",
           (unsigned)tested,
           (unsigned)skipped,
           (unsigned)trials_per_opcode,
           (unsigned)include_illegal,
           *seed);

    return OK;
}
