#include "setup_test.h"
#include "netlist_helpers.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MEM_SIZE 65536u
#define SETUP_PROGRAM_SIZE 35u
#define SETUP_DATA_DEFAULT_VALUE 0x5au
#define SETUP_CYCLES_FROM_DONE_ACCESS 9u

static void set_error(char* error, size_t error_size, const char* fmt, ...)
{
    if ((error != NULL) && (error_size != 0u))
    {
        va_list args;
        va_start(args, fmt);
        (void)vsnprintf(error, error_size, fmt, args);
        va_end(args);
    }
}

static uint8_t clean_p(uint8_t p)
{
    return (uint8_t)((p & 0xcfu) | 0x20u);
}

static uint8_t norm_p(uint8_t p)
{
    return clean_p(p);
}

static void format_regs(char* out, size_t out_size, const regs_t* regs)
{
    if ((out != NULL) && (out_size != 0u) && (regs != NULL))
    {
        (void)snprintf(out,
                       out_size,
                       "PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X",
                       regs->pc,
                       regs->a,
                       regs->x,
                       regs->y,
                       regs->s,
                       norm_p(regs->p));
    }
}

static uint8_t stack_at(const uint8_t* mem, uint8_t s, uint8_t offset)
{
    return mem[(uint16_t)(0x0100u | (uint8_t)(s + offset))];
}


static void put16(uint8_t* mem, uint16_t addr, uint16_t value)
{
    mem[addr] = (uint8_t)(value & 0xffu);
    mem[(uint16_t)(addr + 1u)] = (uint8_t)((value >> 8u) & 0xffu);
}

static void emit_abs(uint8_t* mem, uint16_t* pc, uint8_t opcode, uint16_t address)
{
    mem[*pc] = opcode;
    *pc = (uint16_t)(*pc + 1u);
    mem[*pc] = (uint8_t)(address & 0xffu);
    *pc = (uint16_t)(*pc + 1u);
    mem[*pc] = (uint8_t)((address >> 8u) & 0xffu);
    *pc = (uint16_t)(*pc + 1u);
}


static int service_qe(qe6502_cpu_t* cpu,
                      qe6502_tick_t* tick,
                      uint8_t* mem,
                      char* error,
                      size_t error_size)
{
    const uint16_t addr = QE6502_ADDRESS(*tick);
    uint8_t data = 0u;

    if (QE6502_IS_NOT_OK(*tick))
    {
        set_error(error, error_size, "qe6502 error: %s", qe6502_error_string(cpu));
        return ERR_CPU;
    }

    if (QE6502_IS_WRITING(*tick))
    {
        data = QE6502_DATA(*tick);
        mem[addr] = data;
    }
    else
    {
        data = mem[addr];
    }

    *tick = qe6502_tick(cpu, data);

    if (QE6502_IS_NOT_OK(*tick))
    {
        set_error(error, error_size, "qe6502 error after tick: %s", qe6502_error_string(cpu));
        return ERR_CPU;
    }

    return OK;
}

static int check_qe_regs(qe6502_cpu_t* cpu,
                         const uint8_t* mem,
                         uint16_t code_addr,
                         uint16_t data_addr,
                         const regs_t* expected,
                         char* error,
                         size_t error_size)
{
    const qe6502_state_t state = qe6502_dump_state(cpu);
    const uint16_t pc = QE6502_PC(state);
    const uint8_t a = QE6502_A(state);
    const uint8_t x = QE6502_X(state);
    const uint8_t y = QE6502_Y(state);
    const uint8_t s = QE6502_S(state);
    const uint8_t p = norm_p(QE6502_P(state));

    if ((pc != expected->pc) ||
        (a != expected->a) ||
        (x != expected->x) ||
        (y != expected->y) ||
        (s != expected->s) ||
        (p != norm_p(expected->p)))
    {
        const uint8_t rs = (uint8_t)(expected->s - 3u);
        set_error(error,
                  error_size,
                  "processor=qe setup mismatch: "
                  "got[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] "
                  "expected[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] "
                  "code=%04X data=%04X done=%02X "
                  "setup_data[A=%02X X=%02X Y=%02X P=%02X S=%02X PCL=%02X PCH=%02X RTI_SP=%02X] "
                  "stack[SP0=%02X @%04X=%02X @%04X=%02X @%04X=%02X] "
                  "reset_vector=%02X%02X",
                  pc,
                  a,
                  x,
                  y,
                  s,
                  p,
                  expected->pc,
                  expected->a,
                  expected->x,
                  expected->y,
                  expected->s,
                  norm_p(expected->p),
                  code_addr,
                  data_addr,
                  mem[data_addr],
                  mem[(uint16_t)(data_addr + SETUP_DATA_A)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_X)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_Y)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_P)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_S)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_PCL)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_PCH)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_RTI_SP)],
                  rs,
                  (uint16_t)(0x0100u | (uint8_t)(expected->s - 2u)),
                  stack_at(mem, expected->s, (uint8_t)-2),
                  (uint16_t)(0x0100u | (uint8_t)(expected->s - 1u)),
                  stack_at(mem, expected->s, (uint8_t)-1),
                  (uint16_t)(0x0100u | expected->s),
                  stack_at(mem, expected->s, 0u),
                  mem[0xfffdu],
                  mem[0xfffcu]);
        return ERR_REG;
    }

    return OK;
}

static int check_perfect_regs(state_t* cpu,
                              const uint8_t* mem,
                              uint16_t code_addr,
                              uint16_t data_addr,
                              const regs_t* expected,
                              char* error,
                              size_t error_size)
{
    const uint16_t pc = readPC(cpu);
    const uint8_t a = readA(cpu);
    const uint8_t x = readX(cpu);
    const uint8_t y = readY(cpu);
    const uint8_t s = readSP(cpu);
    const uint8_t p = norm_p(readP(cpu));

    if ((pc != expected->pc) ||
        (a != expected->a) ||
        (x != expected->x) ||
        (y != expected->y) ||
        (s != expected->s) ||
        (p != norm_p(expected->p)))
    {
        const uint8_t rs = (uint8_t)(expected->s - 3u);
        set_error(error,
                  error_size,
                  "processor=perfect setup mismatch: "
                  "got[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X AB=%04X DB=%02X RW=%u half=%lu] "
                  "expected[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] "
                  "code=%04X data=%04X done=%02X "
                  "setup_data[A=%02X X=%02X Y=%02X P=%02X S=%02X PCL=%02X PCH=%02X RTI_SP=%02X] "
                  "stack[SP0=%02X @%04X=%02X @%04X=%02X @%04X=%02X] "
                  "reset_vector=%02X%02X",
                  pc,
                  a,
                  x,
                  y,
                  s,
                  p,
                  readAddressBus(cpu),
                  readDataBus(cpu),
                  (unsigned)readRW(cpu),
                  cycle,
                  expected->pc,
                  expected->a,
                  expected->x,
                  expected->y,
                  expected->s,
                  norm_p(expected->p),
                  code_addr,
                  data_addr,
                  mem[data_addr],
                  mem[(uint16_t)(data_addr + SETUP_DATA_A)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_X)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_Y)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_P)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_S)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_PCL)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_PCH)],
                  mem[(uint16_t)(data_addr + SETUP_DATA_RTI_SP)],
                  rs,
                  (uint16_t)(0x0100u | (uint8_t)(expected->s - 2u)),
                  stack_at(mem, expected->s, (uint8_t)-2),
                  (uint16_t)(0x0100u | (uint8_t)(expected->s - 1u)),
                  stack_at(mem, expected->s, (uint8_t)-1),
                  (uint16_t)(0x0100u | expected->s),
                  stack_at(mem, expected->s, 0u),
                  mem[0xfffdu],
                  mem[0xfffcu]);
        return ERR_REG;
    }

    return OK;
}

void randomize_regs(regs_t* regs, uint32_t* seed)
{
    if ((regs == NULL) || (seed == NULL))
    {
        return;
    }

    regs->pc = (uint16_t)(rnd_next(seed) & 0x7fffu);
    regs->a = (uint8_t)(rnd_next(seed) & 0xffu);
    regs->x = (uint8_t)(rnd_next(seed) & 0xffu);
    regs->y = (uint8_t)(rnd_next(seed) & 0xffu);
    regs->s = (uint8_t)(rnd_next(seed) & 0xffu);
    regs->p = clean_p((uint8_t)(rnd_next(seed) & 0xffu));
}

int inject_setup_program(uint8_t* mem,
                         uint16_t code_addr,
                         uint16_t data_addr,
                         const regs_t* regs)
{
    uint16_t pc = code_addr;

    if ((mem == NULL) || (regs == NULL))
    {
        return ERR_ARG;
    }

    if (((uint32_t)code_addr + (uint32_t)SETUP_PROGRAM_SIZE - 1u) > (uint32_t)SETUP_CODE_END)
    {
        return ERR_ARG;
    }

    mem[(uint16_t)(data_addr + SETUP_DATA_DONE)] = SETUP_DATA_DEFAULT_VALUE;
    mem[(uint16_t)(data_addr + SETUP_DATA_A)] = regs->a;
    mem[(uint16_t)(data_addr + SETUP_DATA_X)] = regs->x;
    mem[(uint16_t)(data_addr + SETUP_DATA_Y)] = regs->y;
    mem[(uint16_t)(data_addr + SETUP_DATA_P)] = clean_p(regs->p);
    mem[(uint16_t)(data_addr + SETUP_DATA_S)] = regs->s;
    mem[(uint16_t)(data_addr + SETUP_DATA_PCL)] = (uint8_t)(regs->pc & 0xffu);
    mem[(uint16_t)(data_addr + SETUP_DATA_PCH)] = (uint8_t)((regs->pc >> 8u) & 0xffu);
    mem[(uint16_t)(data_addr + SETUP_DATA_RTI_SP)] = (uint8_t)(regs->s - 3u);

    put16(mem, 0xfffcu, code_addr);

    /* LDX data.rti_sp */
    emit_abs(mem, &pc, 0xaeu, (uint16_t)(data_addr + SETUP_DATA_RTI_SP));
    /* TXS */
    mem[pc] = 0x9au;
    pc = (uint16_t)(pc + 1u);

    /* stack frame for RTI: P, PCL, PCH at (S-2), (S-1), S.
       Do not use absolute,X here: 6502 stack pulls wrap inside page $01,
       while absolute,X does not wrap from $01FF to $0100. */
    emit_abs(mem, &pc, 0xadu, (uint16_t)(data_addr + SETUP_DATA_P));
    emit_abs(mem, &pc, 0x8du, (uint16_t)(0x0100u | (uint8_t)(regs->s - 2u)));

    emit_abs(mem, &pc, 0xadu, (uint16_t)(data_addr + SETUP_DATA_PCL));
    emit_abs(mem, &pc, 0x8du, (uint16_t)(0x0100u | (uint8_t)(regs->s - 1u)));

    emit_abs(mem, &pc, 0xadu, (uint16_t)(data_addr + SETUP_DATA_PCH));
    emit_abs(mem, &pc, 0x8du, (uint16_t)(0x0100u | regs->s));

    /* final A/X/Y */
    emit_abs(mem, &pc, 0xadu, (uint16_t)(data_addr + SETUP_DATA_A));
    emit_abs(mem, &pc, 0xaeu, (uint16_t)(data_addr + SETUP_DATA_X));
    emit_abs(mem, &pc, 0xacu, (uint16_t)(data_addr + SETUP_DATA_Y));

    /* marker and transfer to final PC/P/S */
    emit_abs(mem, &pc, 0xeeu, (uint16_t)(data_addr + SETUP_DATA_DONE));
    mem[pc] = 0x40u; /* RTI */
    pc = (uint16_t)(pc + 1u);

    return ((uint16_t)(pc - code_addr) == SETUP_PROGRAM_SIZE) ? OK : ERR_INIT;
}

int inject_setup_program_both(uint8_t* qe_mem,
                              uint8_t* perfect_mem,
                              uint16_t code_addr,
                              uint16_t data_addr,
                              const regs_t* regs)
{
    int rc;

    if ((qe_mem == NULL) || (perfect_mem == NULL) || (regs == NULL))
    {
        return ERR_ARG;
    }

    rc = inject_setup_program(qe_mem, code_addr, data_addr, regs);
    if (rc != OK)
    {
        return rc;
    }

    memcpy(perfect_mem, qe_mem, MEM_SIZE);
    return OK;
}

static int run_setup_qe_impl(qe6502_cpu_t* cpu,
                             uint8_t* qe_mem,
                             uint16_t code_addr,
                             uint16_t data_addr,
                             const regs_t* regs,
                             qe6502_tick_t* out_tick,
                             char* error,
                             size_t error_size)
{
    qe6502_tick_t tick;
    uint32_t i;
    uint8_t seen_marker = 0u;
    uint32_t after_marker = 0u;

    if ((cpu == NULL) || (qe_mem == NULL) || (regs == NULL))
    {
        set_error(error, error_size, "invalid run_setup_qe argument");
        return ERR_ARG;
    }

    tick = qe6502_reset(cpu, (uint8_t)QE6502_MODEL_MOS);

    if (QE6502_IS_NOT_OK(tick))
    {
        set_error(error, error_size, "qe6502 reset failed: %s", qe6502_error_string(cpu));
        return ERR_CPU;
    }

    for (i = 0u; i < 512u; ++i)
    {
        if ((seen_marker == 0u) &&
            (QE6502_ADDRESS(tick) == data_addr))
        {
            seen_marker = 1u;
        }

        {
            const int rc = service_qe(cpu, &tick, qe_mem, error, error_size);
            if (rc != OK)
            {
                return rc;
            }
        }

        if (seen_marker != 0u)
        {
            ++after_marker;
            if (after_marker == SETUP_CYCLES_FROM_DONE_ACCESS)
            {
                if (qe_mem[data_addr] != (uint8_t)(SETUP_DATA_DEFAULT_VALUE + 1u))
                {
                    set_error(error,
                              error_size,
                              "processor=qe setup marker mismatch: code=%04X data=%04X got=%02X expected=%02X tick[AB=%04X WR=%u DATA=%02X START=%u DONE=%u]",
                              code_addr,
                              data_addr,
                              qe_mem[data_addr],
                              (uint8_t)(SETUP_DATA_DEFAULT_VALUE + 1u),
                              QE6502_ADDRESS(tick),
                              (unsigned)QE6502_IS_WRITING(tick),
                              QE6502_DATA(tick),
                              (unsigned)QE6502_IS_STARTING(tick),
                              (unsigned)QE6502_IS_INSTR_DONE(tick));
                    return ERR_CPU;
                }

                {
                    const qe6502_tick_t last_tick = qe6502_last_tick(cpu);
                    if (!QE6502_IS_INSTR_DONE(last_tick))
                    {
                        (void)snprintf(
                            error,
                            error_size,
                            "processor=qe setup mismatch: final instruction not done "
                            "last_tick=%08X code=%04X data=%04X "
                            "expected[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X]",
                            (unsigned)last_tick,
                            code_addr,
                            data_addr,
                            regs->pc,
                            regs->a,
                            regs->x,
                            regs->y,
                            regs->s,
                            regs->p);

                        return ERR_CPU;
                    }
                }

                {
                    const int rc = check_qe_regs(cpu, qe_mem, code_addr, data_addr, regs, error, error_size);
                    if (rc != OK)
                    {
                        return rc;
                    }
                }

                if (out_tick != NULL)
                {
                    *out_tick = tick;
                }

                return OK;
            }
        }
    }

    set_error(error, error_size, "processor=qe setup timeout: code=%04X data=%04X seen_marker=%u after_marker=%u tick[AB=%04X WR=%u DATA=%02X START=%u DONE=%u]", code_addr, data_addr, (unsigned)seen_marker, (unsigned)after_marker, QE6502_ADDRESS(tick), (unsigned)QE6502_IS_WRITING(tick), QE6502_DATA(tick), (unsigned)QE6502_IS_STARTING(tick), (unsigned)QE6502_IS_INSTR_DONE(tick));
    return ERR_TIMEOUT;
}

int run_setup_qe(qe6502_cpu_t* cpu,
                 uint8_t* qe_mem,
                 uint16_t code_addr,
                 uint16_t data_addr,
                 const regs_t* regs,
                 char* error,
                 size_t error_size)
{
    return run_setup_qe_impl(cpu,
                             qe_mem,
                             code_addr,
                             data_addr,
                             regs,
                             NULL,
                             error,
                             error_size);
}

int run_setup_qe_fetch(qe6502_cpu_t* cpu,
                       uint8_t* qe_mem,
                       uint16_t code_addr,
                       uint16_t data_addr,
                       const regs_t* regs,
                       qe6502_tick_t* out_tick,
                       char* error,
                       size_t error_size)
{
    if (out_tick == NULL)
    {
        set_error(error, error_size, "invalid run_setup_qe_fetch argument");
        return ERR_ARG;
    }

    return run_setup_qe_impl(cpu,
                             qe_mem,
                             code_addr,
                             data_addr,
                             regs,
                             out_tick,
                             error,
                             error_size);
}

int run_setup_perfect(state_t** out_cpu,
                      uint8_t* perfect_mem,
                      uint16_t code_addr,
                      uint16_t data_addr,
                      const regs_t* regs,
                      char* error,
                      size_t error_size)
{
    state_t* cpu = NULL;
    uint16_t pc = 0u;
    uint32_t i;
    uint8_t seen_marker = 0u;
    uint32_t after_marker = 0u;
    int rc;

    if ((out_cpu == NULL) || (perfect_mem == NULL) || (regs == NULL))
    {
        set_error(error, error_size, "invalid run_setup_perfect argument");
        return ERR_ARG;
    }

    *out_cpu = NULL;

    rc = init_perfect_to_fetch(&cpu, &pc, 0u, error, error_size);
    if (rc != OK)
    {
        return rc;
    }

    if (pc != code_addr)
    {
        set_error(error, error_size, "perfect setup fetch pc mismatch: got=%04X expected=%04X", pc, code_addr);
        destroyChip(cpu);
        return ERR_INIT;
    }

    for (i = 0u; i < 512u; ++i)
    {
        if ((seen_marker == 0u) &&
            (readAddressBus(cpu) == data_addr))
        {
            seen_marker = 1u;
        }

        step(cpu);
        step(cpu);

        if (seen_marker != 0u)
        {
            ++after_marker;
            if (after_marker == SETUP_CYCLES_FROM_DONE_ACCESS)
            {
                if (perfect_mem[data_addr] != (uint8_t)(SETUP_DATA_DEFAULT_VALUE + 1u))
                {
                    set_error(error,
                              error_size,
                              "processor=perfect setup marker mismatch: code=%04X data=%04X got=%02X expected=%02X state[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]",
                              code_addr,
                              data_addr,
                              perfect_mem[data_addr],
                              (uint8_t)(SETUP_DATA_DEFAULT_VALUE + 1u),
                              readPC(cpu),
                              readAddressBus(cpu),
                              readDataBus(cpu),
                              (unsigned)readRW(cpu),
                              cycle);
                    destroyChip(cpu);
                    return ERR_CPU;
                }

                rc = check_perfect_regs(cpu, perfect_mem, code_addr, data_addr, regs, error, error_size);
                if (rc != OK)
                {
                    destroyChip(cpu);
                    return rc;
                }

                *out_cpu = cpu;
                return OK;
            }
        }
    }

    set_error(error, error_size, "processor=perfect setup timeout: code=%04X data=%04X seen_marker=%u after_marker=%u state[PC=%04X AB=%04X DB=%02X RW=%u half=%lu]", code_addr, data_addr, (unsigned)seen_marker, (unsigned)after_marker, readPC(cpu), readAddressBus(cpu), readDataBus(cpu), (unsigned)readRW(cpu), cycle);
    destroyChip(cpu);
    return ERR_TIMEOUT;
}

int run_setup_pair_test(qe6502_cpu_t* qe_cpu,
                        uint8_t* qe_mem,
                        uint8_t* perfect_mem,
                        uint16_t code_addr,
                        uint16_t data_addr,
                        const regs_t* regs,
                        char* error,
                        size_t error_size)
{
    state_t* perfect_cpu = NULL;
    int rc;

    if ((qe_cpu == NULL) || (qe_mem == NULL) || (perfect_mem == NULL) || (regs == NULL))
    {
        set_error(error, error_size, "invalid run_setup_pair_test argument");
        return ERR_ARG;
    }

    rc = inject_setup_program_both(qe_mem, perfect_mem, code_addr, data_addr, regs);
    if (rc != OK)
    {
        set_error(error, error_size, "inject_setup_program failed");
        return rc;
    }

    rc = run_setup_qe(qe_cpu, qe_mem, code_addr, data_addr, regs, error, error_size);
    if (rc != OK)
    {
        return rc;
    }

    rc = run_setup_perfect(&perfect_cpu, perfect_mem, code_addr, data_addr, regs, error, error_size);
    if (rc != OK)
    {
        return rc;
    }

    destroyChip(perfect_cpu);
    return OK;
}

int run_random_setup_tests(qe6502_cpu_t* qe_cpu,
                           uint8_t* qe_mem,
                           uint8_t* perfect_mem,
                           uint32_t* seed,
                           uint32_t count,
                           char* error,
                           size_t error_size)
{
    uint32_t i;

    if ((qe_cpu == NULL) || (qe_mem == NULL) || (perfect_mem == NULL) || (seed == NULL))
    {
        set_error(error, error_size, "invalid run_random_setup_tests argument");
        return ERR_ARG;
    }

    for (i = 0u; i < count; ++i)
    {
        regs_t regs;
        char regs_text[96];
        const uint32_t trial_seed = *seed;
        const uint16_t code_addr = SETUP_CODE_ADDR;
        const uint16_t data_addr = SETUP_DATA_ADDR;
        const int mem_rc = fill_random_memory(qe_mem, perfect_mem, seed);

        if (mem_rc != OK)
        {
            set_error(error, error_size, "fill_random_memory failed");
            return mem_rc;
        }

        randomize_regs(&regs, seed);
        format_regs(regs_text, sizeof(regs_text), &regs);

        {
            const int rc = run_setup_pair_test(qe_cpu,
                                               qe_mem,
                                               perfect_mem,
                                               code_addr,
                                               data_addr,
                                               &regs,
                                               error,
                                               error_size);
            if (rc != OK)
            {
                char prefix[256];
                (void)snprintf(prefix,
                               sizeof(prefix),
                               "setup trial=%u seed_before=%08X seed_after=%08X code=%04X data=%04X regs[%s]: ",
                               (unsigned)i,
                               trial_seed,
                               *seed,
                               code_addr,
                               data_addr,
                               regs_text);
                if ((error != NULL) && (error_size != 0u))
                {
                    char old[1536];
                    (void)snprintf(old, sizeof(old), "%s", error);
                    (void)snprintf(error, error_size, "%s%s", prefix, old);
                }
                return rc;
            }
        }
    }

    return OK;
}
