#include "opcode_test.h"
#include "netlist_helpers.h"
#include "setup_test.h"

#include <stdio.h>
#include <string.h>

#define PROGRAM_MAX_ADDR 0x8000u

static uint8_t ranges_overlap(uint16_t a, size_t a_size, uint16_t b, size_t b_size)
{
    const uint32_t a0 = a;
    const uint32_t a1 = a0 + (uint32_t)a_size;
    const uint32_t b0 = b;
    const uint32_t b1 = b0 + (uint32_t)b_size;

    return (uint8_t)((a0 < b1) && (b0 < a1));
}

static int choose_non_overlapping_pc(regs_t* regs,
                                     uint16_t code_addr,
                                     size_t program_size,
                                     uint32_t* seed,
                                     char* error,
                                     size_t error_size)
{
    uint32_t i;

    if ((regs == NULL) || (seed == NULL) || (program_size == 0u))
    {
        return ERR_ARG;
    }

    if (program_size >= PROGRAM_MAX_ADDR)
    {
        snprintf(error, error_size, "program too large: size=%u", (unsigned)program_size);
        return ERR_ARG;
    }

    for (i = 0u; i < 64u; ++i)
    {
        regs->pc = (uint16_t)(rnd_next(seed) % (PROGRAM_MAX_ADDR - (uint32_t)program_size));

        if (!ranges_overlap(regs->pc, program_size, code_addr, (size_t)(SETUP_CODE_END - SETUP_CODE_ADDR + 1u)))
        {
            return OK;
        }
    }

    snprintf(error,
             error_size,
             "could not choose non-overlapping pc: code=%04X code_size=%u program_size=%u",
             code_addr,
             (unsigned)(SETUP_CODE_END - SETUP_CODE_ADDR + 1u),
             (unsigned)program_size);
    return ERR_INIT;
}

static void copy_program(uint8_t* qe_mem,
                         uint8_t* perfect_mem,
                         uint16_t pc,
                         const uint8_t* program,
                         size_t program_size)
{
    memcpy(&qe_mem[pc], program, program_size);
    memcpy(&perfect_mem[pc], program, program_size);
}

int run_program_test_ex(qe6502_cpu_t* cpu,
                        uint8_t* qe_mem,
                        uint8_t* perfect_mem,
                        const uint8_t* program,
                        size_t program_size,
                        uint32_t instruction_count,
                        uint8_t allow_illegal_end,
                        uint32_t* seed,
                        char* error,
                        size_t error_size)
{
    uint16_t code_addr = SETUP_CODE_ADDR;
    qe6502_tick_t tick = 0u;
    state_t* perfect_cpu = NULL;
    regs_t regs;
    uint32_t i;
    int rc;

    if ((cpu == NULL) ||
        (qe_mem == NULL) ||
        (perfect_mem == NULL) ||
        (program == NULL) ||
        (program_size == 0u) ||
        (instruction_count == 0u) ||
        (seed == NULL))
    {
        snprintf(error, error_size, "invalid run_program_test argument");
        return ERR_ARG;
    }

    rc = randomize_vectors(qe_mem, perfect_mem, seed, NULL);
    if (rc != OK)
    {
        snprintf(error, error_size, "randomize_vectors failed");
        return rc;
    }

    code_addr = SETUP_CODE_ADDR;
    randomize_regs(&regs, seed);

    rc = choose_non_overlapping_pc(&regs, code_addr, program_size, seed, error, error_size);
    if (rc != OK)
    {
        return rc;
    }

    rc = inject_setup_program_both(qe_mem, perfect_mem, code_addr, SETUP_DATA_ADDR, &regs);
    if (rc != OK)
    {
        snprintf(error,
                 error_size,
                 "inject_setup_program_both failed: code=%04X data=%04X pc=%04X size=%u",
                 code_addr,
                 SETUP_DATA_ADDR,
                 regs.pc,
                 (unsigned)program_size);
        return rc;
    }

    copy_program(qe_mem, perfect_mem, regs.pc, program, program_size);

    rc = run_setup_qe_fetch(cpu,
                            qe_mem,
                            code_addr,
                            SETUP_DATA_ADDR,
                            &regs,
                            &tick,
                            error,
                            error_size);
    if (rc != OK)
    {
        char detail[1536];

        (void)snprintf(detail,
                       sizeof(detail),
                       "qe bootstrap failed: code=%04X data=%04X pc=%04X size=%u instr=%u: %s",
                       code_addr,
                       SETUP_DATA_ADDR,
                       regs.pc,
                       (unsigned)program_size,
                       (unsigned)instruction_count,
                       error);
        (void)snprintf(error, error_size, "%s", detail);
        return rc;
    }

    rc = run_setup_perfect(&perfect_cpu,
                           perfect_mem,
                           code_addr,
                           SETUP_DATA_ADDR,
                           &regs,
                           error,
                           error_size);
    if (rc != OK)
    {
        char detail[1536];

        (void)snprintf(detail,
                       sizeof(detail),
                       "perfect bootstrap failed: code=%04X data=%04X pc=%04X size=%u instr=%u: %s",
                       code_addr,
                       SETUP_DATA_ADDR,
                       regs.pc,
                       (unsigned)program_size,
                       (unsigned)instruction_count,
                       error);
        (void)snprintf(error, error_size, "%s", detail);
        return rc;
    }

    for (i = 0u; i < instruction_count; ++i)
    {
        uint8_t ended_on_illegal = 0u;

        rc = compare_instruction_bus_ex(cpu,
                                        &tick,
                                        qe_mem,
                                        perfect_cpu,
                                        perfect_mem,
                                        0u,
                                        allow_illegal_end,
                                        &ended_on_illegal,
                                        error,
                                        error_size);
        if (rc != OK)
        {
            char detail[1536];

            (void)snprintf(detail,
                           sizeof(detail),
                           "program instruction %u failed: code=%04X data=%04X pc=%04X size=%u instr=%u allow_illegal_end=%u qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]: %s",
                           (unsigned)i,
                           code_addr,
                           SETUP_DATA_ADDR,
                           regs.pc,
                           (unsigned)program_size,
                           (unsigned)instruction_count,
                           (unsigned)allow_illegal_end,
                           (unsigned)tick,
                           readPC(perfect_cpu),
                           readAddressBus(perfect_cpu),
                           readDataBus(perfect_cpu),
                           (unsigned)readRW(perfect_cpu),
                           cycle,
                           error);
            (void)snprintf(error, error_size, "%s", detail);
            destroyChip(perfect_cpu);
            return rc;
        }

        if (ended_on_illegal != 0u)
        {
            destroyChip(perfect_cpu);
            return OK;
        }
    }

    rc = compare_final_registers(cpu, perfect_cpu, error, error_size);
    if (rc != OK)
    {
        char detail[1536];

        (void)snprintf(detail,
                       sizeof(detail),
                       "program final register check failed: code=%04X data=%04X pc=%04X size=%u instr=%u qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]: %s",
                       code_addr,
                       SETUP_DATA_ADDR,
                       regs.pc,
                       (unsigned)program_size,
                       (unsigned)instruction_count,
                       (unsigned)tick,
                       readPC(perfect_cpu),
                       readAddressBus(perfect_cpu),
                       readDataBus(perfect_cpu),
                       (unsigned)readRW(perfect_cpu),
                       cycle,
                       error);
        (void)snprintf(error, error_size, "%s", detail);
        destroyChip(perfect_cpu);
        return rc;
    }

    destroyChip(perfect_cpu);
    return OK;
}

int run_program_test(qe6502_cpu_t* cpu,
                     uint8_t* qe_mem,
                     uint8_t* perfect_mem,
                     const uint8_t* program,
                     size_t program_size,
                     uint32_t instruction_count,
                     uint32_t* seed,
                     char* error,
                     size_t error_size)
{
    return run_program_test_ex(cpu,
                               qe_mem,
                               perfect_mem,
                               program,
                               program_size,
                               instruction_count,
                               0u,
                               seed,
                               error,
                               error_size);
}

int run_opcode_test_ex(qe6502_cpu_t* cpu,
                       uint8_t* qe_mem,
                       uint8_t* perfect_mem,
                       uint8_t opcode,
                       uint8_t allow_illegal_end,
                       uint32_t* seed,
                       char* error,
                       size_t error_size)
{
    return run_program_test_ex(cpu,
                               qe_mem,
                               perfect_mem,
                               &opcode,
                               1u,
                               1u,
                               allow_illegal_end,
                               seed,
                               error,
                               error_size);
}


int run_opcode_test(qe6502_cpu_t* cpu,
                    uint8_t* qe_mem,
                    uint8_t* perfect_mem,
                    uint8_t opcode,
                    uint32_t* seed,
                    char* error,
                    size_t error_size)
{
    return run_opcode_test_ex(cpu,
                              qe_mem,
                              perfect_mem,
                              opcode,
                              0u,
                              seed,
                              error,
                              error_size);
}
