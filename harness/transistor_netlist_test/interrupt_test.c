#include "interrupt_test.h"

#include "netlist_helpers.h"
#include "setup_test.h"
#include "third_party/perfect6502/perfect6502.h"
#include "third_party/perfect6502/types.h"
#include "third_party/perfect6502/netlist_sim.h"

#include <stdio.h>
#include <string.h>

#define IRQ_NODE               ((nodenum_t)103u)

#define IRQ_TEST_LDA_PC        0x2000u
#define IRQ_TEST_DATA_ADDR     0x3456u
#define IRQ_TEST_VECTOR        0x4000u
#define IRQ_TEST_MAX_CYCLES    96u
#define IRQ_TEST_CASES         6u

static void put16(uint8_t* mem, uint16_t addr, uint16_t value)
{
    mem[addr] = (uint8_t)(value & 0xffu);
    mem[(uint16_t)(addr + 1u)] = (uint8_t)((value >> 8u) & 0xffu);
}

static void emit_abs(uint8_t* mem, uint16_t* pc, uint8_t opcode, uint16_t addr)
{
    mem[*pc] = opcode;
    *pc = (uint16_t)(*pc + 1u);
    mem[*pc] = (uint8_t)(addr & 0xffu);
    *pc = (uint16_t)(*pc + 1u);
    mem[*pc] = (uint8_t)((addr >> 8u) & 0xffu);
    *pc = (uint16_t)(*pc + 1u);
}

static uint8_t norm_p(uint8_t p)
{
    return (uint8_t)((p & 0xefu) | 0x20u);
}

static void fill_irq_handler(uint8_t* mem)
{
    uint16_t i;

    for (i = 0u; i < 32u; ++i)
    {
        mem[(uint16_t)(IRQ_TEST_VECTOR + i)] = 0xeau; /* NOP */
    }
}

static void set_test_program(uint8_t* mem)
{
    put16(mem, 0xfffcu, SETUP_CODE_ADDR);
    put16(mem, 0xfffeu, IRQ_TEST_VECTOR);

    mem[IRQ_TEST_LDA_PC] = 0xadu; /* LDA abs */
    mem[(uint16_t)(IRQ_TEST_LDA_PC + 1u)] = (uint8_t)(IRQ_TEST_DATA_ADDR & 0xffu);
    mem[(uint16_t)(IRQ_TEST_LDA_PC + 2u)] = (uint8_t)((IRQ_TEST_DATA_ADDR >> 8u) & 0xffu);
    mem[(uint16_t)(IRQ_TEST_LDA_PC + 3u)] = 0xaau; /* TAX: non-NOP, detects too-late IRQ */
    mem[(uint16_t)(IRQ_TEST_LDA_PC + 4u)] = 0xeau; /* safe filler */

    mem[IRQ_TEST_DATA_ADDR] = 0xc3u;

    fill_irq_handler(mem);
}

static void inject_irq_bootstrap(uint8_t* mem, const regs_t* regs)
{
    uint16_t pc = SETUP_CODE_ADDR;
    const uint16_t p_stack_addr = (uint16_t)(0x0100u | regs->s);

    mem[(uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_A)] = regs->a;
    mem[(uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_X)] = regs->x;
    mem[(uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_Y)] = regs->y;
    mem[(uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_P)] = regs->p;
    mem[(uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_S)] = regs->s;

    /*
       Test-only bootstrap without RTI:
       - writes P to the stack slot that PLP will pull,
       - sets S to final_s - 1,
       - restores A/X/Y,
       - PLP restores final P and increments S to final_s,
       - JMP enters the tested instruction stream.

       This avoids RTI-specific interrupt suppression before the first tested
       instruction.
    */

    emit_abs(mem, &pc, 0xadu, (uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_P)); /* LDA P */
    emit_abs(mem, &pc, 0x8du, p_stack_addr);                              /* STA stack */
    emit_abs(mem, &pc, 0xaeu, (uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_S)); /* LDX S */
    mem[pc] = 0xcau;                                                       /* DEX -> S - 1 */
    pc = (uint16_t)(pc + 1u);
    mem[pc] = 0x9au;                                                       /* TXS */
    pc = (uint16_t)(pc + 1u);

    emit_abs(mem, &pc, 0xadu, (uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_A)); /* LDA A */
    emit_abs(mem, &pc, 0xaeu, (uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_X)); /* LDX X */
    emit_abs(mem, &pc, 0xacu, (uint16_t)(SETUP_DATA_ADDR + SETUP_DATA_Y)); /* LDY Y */

    mem[pc] = 0x28u;                                                       /* PLP */
    pc = (uint16_t)(pc + 1u);

    mem[pc] = 0x4cu;                                                       /* JMP target */
    pc = (uint16_t)(pc + 1u);
    mem[pc] = (uint8_t)(regs->pc & 0xffu);
    pc = (uint16_t)(pc + 1u);
    mem[pc] = (uint8_t)((regs->pc >> 8u) & 0xffu);
}

static void set_irq_pin_perfect(state_t* cpu, uint8_t asserted)
{
    setNode(cpu, IRQ_NODE, (BOOL)(asserted != 0u ? 0u : 1u));
    recalcNodeList(cpu);
}

static int set_irq_pin_qe(qe6502_cpu_t* cpu,
                          qe6502_tick_t* tick,
                          uint8_t asserted,
                          char* error,
                          size_t error_size)
{
    qe6502_tick_t t;

    if ((cpu == NULL) || (tick == NULL))
    {
        if ((error != NULL) && (error_size != 0u))
        {
            (void)snprintf(error, error_size, "invalid set_irq_pin_qe argument");
        }
        return ERR_ARG;
    }

    t = qe6502_set_irq(cpu, asserted);
    if (QE6502_IS_NOT_OK(t))
    {
        if ((error != NULL) && (error_size != 0u))
        {
            (void)snprintf(error, error_size, "qe6502_set_irq failed: %s", qe6502_error_string(cpu));
        }
        return ERR_CPU;
    }

    *tick = t;
    return OK;
}

static int check_bootstrap_regs(qe6502_cpu_t* qe_cpu,
                                state_t* perfect_cpu,
                                const regs_t* regs,
                                char* error,
                                size_t error_size)
{
    const qe6502_state_t qe_state = qe6502_dump_state(qe_cpu);

    const uint16_t qe_pc = QE6502_PC(qe_state);
    const uint8_t qe_a = QE6502_A(qe_state);
    const uint8_t qe_x = QE6502_X(qe_state);
    const uint8_t qe_y = QE6502_Y(qe_state);
    const uint8_t qe_s = QE6502_S(qe_state);
    const uint8_t qe_p = norm_p(QE6502_P(qe_state));

    const uint16_t pf_pc = readPC(perfect_cpu);
    const uint8_t pf_a = readA(perfect_cpu);
    const uint8_t pf_x = readX(perfect_cpu);
    const uint8_t pf_y = readY(perfect_cpu);
    const uint8_t pf_s = readSP(perfect_cpu);
    const uint8_t pf_p = norm_p(readP(perfect_cpu));

    if ((qe_pc != regs->pc) ||
        (qe_a != regs->a) ||
        (qe_x != regs->x) ||
        (qe_y != regs->y) ||
        (qe_s != regs->s) ||
        (qe_p != norm_p(regs->p)) ||
        (pf_pc != regs->pc) ||
        (pf_a != regs->a) ||
        (pf_x != regs->x) ||
        (pf_y != regs->y) ||
        (pf_s != regs->s) ||
        (pf_p != norm_p(regs->p)))
    {
        (void)snprintf(error,
                       error_size,
                       "irq bootstrap register mismatch: "
                       "expected[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] "
                       "qe[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] "
                       "perfect[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X AB=%04X DB=%02X RW=%u half=%lu]",
                       regs->pc,
                       regs->a,
                       regs->x,
                       regs->y,
                       regs->s,
                       norm_p(regs->p),
                       qe_pc,
                       qe_a,
                       qe_x,
                       qe_y,
                       qe_s,
                       qe_p,
                       pf_pc,
                       pf_a,
                       pf_x,
                       pf_y,
                       pf_s,
                       pf_p,
                       readAddressBus(perfect_cpu),
                       readDataBus(perfect_cpu),
                       (unsigned)readRW(perfect_cpu),
                       cycle);
        return ERR_REG;
    }

    return OK;
}

static int prepare_irq_case(qe6502_cpu_t* cpu,
                            uint8_t* qe_mem,
                            uint8_t* perfect_mem,
                            state_t** out_perfect,
                            qe6502_tick_t* out_tick,
                            char* error,
                            size_t error_size)
{
    regs_t regs;
    state_t* perfect_cpu = NULL;
    qe6502_tick_t tick = 0u;
    uint16_t pc = 0u;
    uint32_t i;
    int rc;

    if ((cpu == NULL) ||
        (qe_mem == NULL) ||
        (perfect_mem == NULL) ||
        (out_perfect == NULL) ||
        (out_tick == NULL))
    {
        if ((error != NULL) && (error_size != 0u))
        {
            (void)snprintf(error, error_size, "invalid prepare_irq_case argument");
        }
        return ERR_ARG;
    }

    *out_perfect = NULL;
    *out_tick = 0u;

    regs.pc = IRQ_TEST_LDA_PC;
    regs.a = 0x11u;
    regs.x = 0x22u;
    regs.y = 0x33u;
    regs.s = 0xf0u;
    regs.p = 0x20u; /* I clear, B clear, unused bit set */

    set_test_program(qe_mem);
    set_test_program(perfect_mem);
    inject_irq_bootstrap(qe_mem, &regs);
    inject_irq_bootstrap(perfect_mem, &regs);

    rc = init_qe_to_fetch(cpu, &tick, qe_mem, SETUP_CODE_ADDR, 0u, error, error_size);
    if (rc != OK)
    {
        char detail[1536];

        (void)snprintf(detail, sizeof(detail), "irq lda init qe failed: %s", error);
        (void)snprintf(error, error_size, "%s", detail);
        return rc;
    }

    rc = init_perfect_to_fetch(&perfect_cpu, &pc, 0u, error, error_size);
    if (rc != OK)
    {
        char detail[1536];

        (void)snprintf(detail, sizeof(detail), "irq lda init perfect failed: %s", error);
        (void)snprintf(error, error_size, "%s", detail);
        return rc;
    }

    if (pc != SETUP_CODE_ADDR)
    {
        (void)snprintf(error,
                       error_size,
                       "irq lda bootstrap fetch mismatch: perfect=%04X expected=%04X",
                       pc,
                       SETUP_CODE_ADDR);
        destroyChip(perfect_cpu);
        return ERR_INIT;
    }

    for (i = 0u; i < 64u; ++i)
    {
        if ((!QE6502_IS_WRITING(tick)) && (QE6502_ADDRESS(tick) == IRQ_TEST_LDA_PC))
        {
            rc = check_bootstrap_regs(cpu, perfect_cpu, &regs, error, error_size);
            if (rc != OK)
            {
                destroyChip(perfect_cpu);
                return rc;
            }

            *out_perfect = perfect_cpu;
            *out_tick = tick;
            return OK;
        }

        rc = compare_cycle(cpu,
                           &tick,
                           qe_mem,
                           perfect_cpu,
                           perfect_mem,
                           error,
                           error_size);
        if (rc != OK)
        {
            char detail[1536];

            (void)snprintf(detail,
                           sizeof(detail),
                           "irq lda bootstrap compare failed: step=%u qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]: %s",
                           (unsigned)i,
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
    }

    (void)snprintf(error,
                   error_size,
                   "irq lda bootstrap timeout: qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]",
                   (unsigned)tick,
                   readPC(perfect_cpu),
                   readAddressBus(perfect_cpu),
                   readDataBus(perfect_cpu),
                   (unsigned)readRW(perfect_cpu),
                   cycle);

    destroyChip(perfect_cpu);
    return ERR_TIMEOUT;
}

static int run_irq_case(uint32_t irq_cycle,
                        const uint8_t* base_mem,
                        qe6502_cpu_t* cpu,
                        uint8_t* qe_mem,
                        uint8_t* perfect_mem,
                        char* error,
                        size_t error_size)
{
    qe6502_tick_t tick = 0u;
    state_t* perfect_cpu = NULL;
    uint32_t cycle_index;
    uint8_t irq_asserted = 0u;
    int rc;

    memcpy(qe_mem, base_mem, 65536u);
    memcpy(perfect_mem, base_mem, 65536u);

    rc = prepare_irq_case(cpu, qe_mem, perfect_mem, &perfect_cpu, &tick, error, error_size);
    if (rc != OK)
    {
        char detail[1792];

        (void)snprintf(detail,
                       sizeof(detail),
                       "irq case prepare failed: irq_cycle=%u: %s",
                       (unsigned)irq_cycle,
                       error);
        (void)snprintf(error, error_size, "%s", detail);
        return rc;
    }

    for (cycle_index = 0u; cycle_index < IRQ_TEST_MAX_CYCLES; ++cycle_index)
    {
        if ((irq_asserted == 0u) && (cycle_index == irq_cycle))
        {
            rc = set_irq_pin_qe(cpu, &tick, 1u, error, error_size);
            if (rc != OK)
            {
                destroyChip(perfect_cpu);
                return rc;
            }

            set_irq_pin_perfect(perfect_cpu, 1u);
            irq_asserted = 1u;
        }

        rc = compare_cycle(cpu,
                           &tick,
                           qe_mem,
                           perfect_cpu,
                           perfect_mem,
                           error,
                           error_size);
        if (rc != OK)
        {
            char detail[2048];

            (void)snprintf(detail,
                           sizeof(detail),
                           "irq lda bus compare failed: irq_cycle=%u cycle=%u asserted=%u "
                           "qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]: %s",
                           (unsigned)irq_cycle,
                           (unsigned)cycle_index,
                           (unsigned)irq_asserted,
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

        if ((irq_asserted != 0u) &&
            (!QE6502_IS_WRITING(tick)) &&
            (QE6502_ADDRESS(tick) == IRQ_TEST_VECTOR))
        {
            rc = compare_instruction(cpu,
                                     &tick,
                                     qe_mem,
                                     perfect_cpu,
                                     perfect_mem,
                                     8u,
                                     error,
                                     error_size);
            if (rc != OK)
            {
                char detail[2048];

                (void)snprintf(detail,
                               sizeof(detail),
                               "irq handler nop compare failed: irq_cycle=%u cycle=%u "
                               "qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]: %s",
                               (unsigned)irq_cycle,
                               (unsigned)cycle_index,
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
    }

    (void)snprintf(error,
                   error_size,
                   "irq lda timeout: irq_cycle=%u asserted=%u qe_tick=%08X perfect[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]",
                   (unsigned)irq_cycle,
                   (unsigned)irq_asserted,
                   (unsigned)tick,
                   readPC(perfect_cpu),
                   readAddressBus(perfect_cpu),
                   readDataBus(perfect_cpu),
                   (unsigned)readRW(perfect_cpu),
                   cycle);

    destroyChip(perfect_cpu);
    return ERR_TIMEOUT;
}

int run_irq_lda_abs_timing_test(qe6502_cpu_t* cpu,
                                uint8_t* qe_mem,
                                uint8_t* perfect_mem,
                                uint32_t* seed,
                                char* error,
                                size_t error_size)
{
    static uint8_t base_mem[65536];
    uint32_t irq_cycle;
    int rc;

    if ((cpu == NULL) ||
        (qe_mem == NULL) ||
        (perfect_mem == NULL) ||
        (seed == NULL))
    {
        if ((error != NULL) && (error_size != 0u))
        {
            (void)snprintf(error, error_size, "invalid run_irq_lda_abs_timing_test argument");
        }
        return ERR_ARG;
    }

    rc = fill_random_memory(base_mem, perfect_mem, seed);
    if (rc != OK)
    {
        (void)snprintf(error, error_size, "irq lda fill_random_memory failed");
        return rc;
    }

    for (irq_cycle = 0u; irq_cycle < IRQ_TEST_CASES; ++irq_cycle)
    {
        rc = run_irq_case(irq_cycle,
                          base_mem,
                          cpu,
                          qe_mem,
                          perfect_mem,
                          error,
                          error_size);
        if (rc != OK)
        {
            char detail[2304];

            (void)snprintf(detail,
                           sizeof(detail),
                           "irq lda timing test failed: irq_cycle=%u seed=%08X: %s",
                           (unsigned)irq_cycle,
                           *seed,
                           error);
            (void)snprintf(error, error_size, "%s", detail);
            return rc;
        }
    }

    printf("irq lda abs timing done: cases=%u seed=%08X\n",
           (unsigned)IRQ_TEST_CASES,
           *seed);

    return OK;
}
