#include "netlist_helpers.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <qe/impl_utils.h>

#define MEM_SIZE 65536u

static void set_error(char* error, size_t error_size, const char* fmt, ...)
{
    if ((error != NULL) && (error_size != 0u))
    {
        va_list args;
        va_start(args, fmt);
        qe_vsnprintf(error, error_size, fmt, args);
        va_end(args);
    }
}

uint32_t rnd_next(uint32_t* seed)
{
    uint32_t x;

    if (seed == NULL)
    {
        return 0u;
    }

    x = *seed;
    if (x == 0u)
    {
        x = 0x6d2b79f5u;
    }

    x ^= x << 13u;
    x ^= x >> 17u;
    x ^= x << 5u;

    *seed = x;
    return x;
}

int fill_random_memory(uint8_t* qe_mem,
                       uint8_t* perfect_mem,
                       uint32_t* seed)
{
    size_t i;

    if ((qe_mem == NULL) || (perfect_mem == NULL) || (seed == NULL))
    {
        return ERR_ARG;
    }

    for (i = 0u; i < MEM_SIZE; ++i)
    {
        qe_mem[i] = (uint8_t)(rnd_next(seed) & 0xffu);
    }

    memcpy(perfect_mem, qe_mem, MEM_SIZE);
    return OK;
}

static void set_vector(uint8_t* mem, uint16_t vector, uint16_t target)
{
    mem[vector] = (uint8_t)(target & 0x00ffu);
    mem[(uint16_t)(vector + 1u)] = (uint8_t)((target >> 8u) & 0x00ffu);
}

int randomize_vectors(uint8_t* qe_mem,
                      uint8_t* perfect_mem,
                      uint32_t* seed,
                      uint16_t* reset_pc)
{
    const uint16_t nmi = (uint16_t)(rnd_next(seed) & 0x7fffu);
    const uint16_t rst = (uint16_t)(rnd_next(seed) & 0x7fffu);
    const uint16_t irq = (uint16_t)(rnd_next(seed) & 0x7fffu);

    if ((qe_mem == NULL) || (perfect_mem == NULL) || (seed == NULL))
    {
        return ERR_ARG;
    }

    set_vector(qe_mem, 0xfffau, nmi);
    set_vector(qe_mem, 0xfffcu, rst);
    set_vector(qe_mem, 0xfffeu, irq);

    set_vector(perfect_mem, 0xfffau, nmi);
    set_vector(perfect_mem, 0xfffcu, rst);
    set_vector(perfect_mem, 0xfffeu, irq);

    if (reset_pc != NULL)
    {
        *reset_pc = rst;
    }

    return OK;
}

int init_perfect_to_fetch(state_t** out_cpu,
                          uint16_t* out_pc,
                          unsigned long max_half_cycles,
                          char* error,
                          size_t error_size)
{
    const uint16_t fetch_pc = (uint16_t)((uint16_t)memory[0xfffcu] |
                                        ((uint16_t)memory[0xfffdu] << 8u));
    state_t* cpu;
    unsigned long i;

    if (out_cpu == NULL)
    {
        set_error(error, error_size, "out_cpu is null");
        return ERR_ARG;
    }

    *out_cpu = NULL;
    cpu = initAndResetChip();

    if (cpu == NULL)
    {
        set_error(error, error_size, "initAndResetChip failed");
        return ERR_INIT;
    }

    if (max_half_cycles == 0u)
    {
        max_half_cycles = 256u;
    }

    for (i = 0u; i < max_half_cycles; ++i)
    {
        if ((readRW(cpu) != 0u) &&
            (readAddressBus(cpu) == fetch_pc) &&
            (readPC(cpu) == fetch_pc))
        {
            if (out_pc != NULL)
            {
                *out_pc = fetch_pc;
            }

            *out_cpu = cpu;
            return OK;
        }

        step(cpu);
    }

    set_error(error,
              error_size,
              "perfect6502 timeout waiting for opcode fetch: vector=%04X pc=%04X ab=%04X rw=%u half=%lu",
              fetch_pc,
              readPC(cpu),
              readAddressBus(cpu),
              (unsigned)readRW(cpu),
              cycle);

    destroyChip(cpu);
    return ERR_TIMEOUT;
}

static int service_qe_cycle(qe6502_cpu_t* cpu,
                            qe6502_tick_t* tick,
                            uint8_t* qe_mem,
                            qe_step_info_t* info,
                            char* error,
                            size_t error_size)
{
    const uint16_t address = QE6502_ADDRESS(*tick);
    const uint8_t write = QE6502_IS_WRITING(*tick);
    uint8_t data;
    qe6502_tick_t next_tick;

    if (info != NULL)
    {
        info->before = *tick;
        info->after = *tick;
        info->address = address;
        info->data = 0u;
        info->write = write;
        info->illegal = 0u;
        info->error_code = 0u;
    }

    if (QE6502_IS_NOT_OK(*tick))
    {
        const uint16_t err = qe6502_error_code(cpu);

        if (info != NULL)
        {
            info->error_code = err;
            info->illegal = (uint8_t)(err == 1u);
        }

        set_error(error, error_size, "qe6502 error: %s", qe6502_error_string(cpu));
        return ERR_CPU;
    }

    if (write != 0u)
    {
        data = QE6502_DATA(*tick);
        qe_mem[address] = data;
    }
    else
    {
        data = qe_mem[address];
    }

    if (info != NULL)
    {
        info->data = data;
    }

    next_tick = qe6502_tick(cpu, data);
    *tick = next_tick;

    if (info != NULL)
    {
        info->after = next_tick;
    }

    if (QE6502_IS_NOT_OK(next_tick))
    {
        const uint16_t err = qe6502_error_code(cpu);

        if (info != NULL)
        {
            info->error_code = err;
            info->illegal = (uint8_t)(err == 1u);
        }

        set_error(error,
                  error_size,
                  "qe6502 error after tick: %s addr=%04X data=%02X write=%u tick_before=%08X tick_after=%08X err=%u",
                  qe6502_error_string(cpu),
                  address,
                  data,
                  (unsigned)write,
                  (unsigned)(info != NULL ? info->before : 0u),
                  (unsigned)next_tick,
                  (unsigned)err);
        return ERR_CPU;
    }

    return OK;
}

int init_qe_to_fetch(qe6502_cpu_t* cpu,
                     qe6502_tick_t* tick,
                     uint8_t* qe_mem,
                     uint16_t fetch_pc,
                     uint32_t max_cycles,
                     char* error,
                     size_t error_size)
{
    uint32_t i;

    if ((cpu == NULL) || (tick == NULL) || (qe_mem == NULL))
    {
        set_error(error, error_size, "invalid qe init argument");
        return ERR_ARG;
    }

    *tick = qe6502_reset(cpu, (uint8_t)QE6502_MODEL_MOS);

    if (QE6502_IS_NOT_OK(*tick))
    {
        set_error(error, error_size, "qe6502 reset failed: %s", qe6502_error_string(cpu));
        return ERR_CPU;
    }

    if (max_cycles == 0u)
    {
        max_cycles = 32u;
    }

    for (i = 0u; i < max_cycles; ++i)
    {
        if (!QE6502_IS_WRITING(*tick) &&
            !QE6502_IS_STARTING(*tick) &&
            (QE6502_ADDRESS(*tick) == fetch_pc))
        {
            return OK;
        }

        {
            const int rc = service_qe_cycle(cpu, tick, qe_mem, NULL, error, error_size);
            if (rc != OK)
            {
                return rc;
            }
        }
    }

    set_error(error,
              error_size,
              "qe6502 timeout waiting for opcode fetch: vector=%04X ab=%04X wr=%u starting=%u",
              fetch_pc,
              QE6502_ADDRESS(*tick),
              (unsigned)QE6502_IS_WRITING(*tick),
              (unsigned)QE6502_IS_STARTING(*tick));
    return ERR_TIMEOUT;
}

static int compare_bus_request(qe6502_tick_t qe_tick,
                               state_t* perfect_cpu,
                               const uint8_t* qe_mem,
                               const uint8_t* perfect_mem,
                               char* error,
                               size_t error_size)
{
    const uint16_t qe_addr = QE6502_ADDRESS(qe_tick);
    const uint8_t qe_wr = QE6502_IS_WRITING(qe_tick);
    const uint8_t qe_data = qe_wr ? QE6502_DATA(qe_tick) : qe_mem[qe_addr];

    const uint16_t pf_addr = readAddressBus(perfect_cpu);
    const uint8_t pf_wr = (uint8_t)(readRW(perfect_cpu) == 0u ? 1u : 0u);
    const uint8_t pf_data = pf_wr ? 0u : perfect_mem[pf_addr];

    if ((qe_addr != pf_addr) || (qe_wr != pf_wr) || ((!qe_wr) && (qe_data != pf_data)))
    {
        set_error(error,
                  error_size,
                  "bus request mismatch: qe[%c %04X %02X] perfect[%c %04X %02X]",
                  qe_wr ? 'W' : 'R',
                  qe_addr,
                  qe_data,
                  pf_wr ? 'W' : 'R',
                  pf_addr,
                  pf_data);
        return ERR_BUS;
    }

    return OK;
}

static int compare_write_result(qe6502_tick_t qe_tick,
                                const uint8_t* perfect_mem,
                                char* error,
                                size_t error_size)
{
    const uint16_t addr = QE6502_ADDRESS(qe_tick);
    const uint8_t qe_data = QE6502_DATA(qe_tick);
    const uint8_t pf_data = perfect_mem[addr];

    if (qe_data != pf_data)
    {
        set_error(error,
                  error_size,
                  "bus write data mismatch: addr=%04X qe=%02X perfect=%02X",
                  addr,
                  qe_data,
                  pf_data);
        return ERR_BUS;
    }

    return OK;
}

static uint8_t normalize_p(uint8_t p)
{
    return (uint8_t)((p & 0xefu) | 0x20u);
}

int compare_cycle_ex(qe6502_cpu_t* qe_cpu,
                     qe6502_tick_t* qe_tick,
                     uint8_t* qe_mem,
                     state_t* perfect_cpu,
                     uint8_t* perfect_mem,
                     uint8_t allow_illegal_end,
                     uint8_t* ended_on_illegal,
                     qe_step_info_t* step_info,
                     char* error,
                     size_t error_size)
{
    int rc;
    qe_step_info_t local_info;
    const qe6502_tick_t old_tick = (qe_tick != NULL) ? *qe_tick : 0u;
    const uint8_t was_write = QE6502_IS_WRITING(old_tick);

    if (ended_on_illegal != NULL)
    {
        *ended_on_illegal = 0u;
    }

    if ((qe_cpu == NULL) ||
        (qe_tick == NULL) ||
        (qe_mem == NULL) ||
        (perfect_cpu == NULL) ||
        (perfect_mem == NULL))
    {
        set_error(error, error_size, "invalid compare_cycle_ex argument");
        return ERR_ARG;
    }

    if (step_info == NULL)
    {
        step_info = &local_info;
    }

    rc = compare_bus_request(old_tick, perfect_cpu, qe_mem, perfect_mem, error, error_size);
    if (rc != OK)
    {
        return rc;
    }

    rc = service_qe_cycle(qe_cpu, qe_tick, qe_mem, step_info, error, error_size);
    if (rc != OK)
    {
        if ((allow_illegal_end != 0u) && (step_info->illegal != 0u))
        {
            step(perfect_cpu);

            if (was_write != 0u)
            {
                const int wr_rc = compare_write_result(old_tick, perfect_mem, error, error_size);
                if (wr_rc != OK)
                {
                    return wr_rc;
                }
            }

            step(perfect_cpu);

            if (ended_on_illegal != NULL)
            {
                *ended_on_illegal = 1u;
            }

            return OK;
        }

        return rc;
    }

    step(perfect_cpu);

    if (was_write != 0u)
    {
        rc = compare_write_result(old_tick, perfect_mem, error, error_size);
        if (rc != OK)
        {
            return rc;
        }
    }

    step(perfect_cpu);

    return OK;
}

int compare_cycle(qe6502_cpu_t* qe_cpu,
                  qe6502_tick_t* qe_tick,
                  uint8_t* qe_mem,
                  state_t* perfect_cpu,
                  uint8_t* perfect_mem,
                  char* error,
                  size_t error_size)
{
    return compare_cycle_ex(qe_cpu,
                            qe_tick,
                            qe_mem,
                            perfect_cpu,
                            perfect_mem,
                            0u,
                            NULL,
                            NULL,
                            error,
                            error_size);
}

int compare_instruction_bus_ex(qe6502_cpu_t* qe_cpu,
                               qe6502_tick_t* qe_tick,
                               uint8_t* qe_mem,
                               state_t* perfect_cpu,
                               uint8_t* perfect_mem,
                               uint32_t max_cycles,
                               uint8_t allow_illegal_end,
                               uint8_t* ended_on_illegal,
                               char* error,
                               size_t error_size)
{
    uint32_t i;

    if (ended_on_illegal != NULL)
    {
        *ended_on_illegal = 0u;
    }

    if (max_cycles == 0u)
    {
        max_cycles = 64u;
    }

    for (i = 0u; i < max_cycles; ++i)
    {
        uint8_t illegal_end = 0u;
        qe_step_info_t step_info;
        const int rc = compare_cycle_ex(qe_cpu,
                                        qe_tick,
                                        qe_mem,
                                        perfect_cpu,
                                        perfect_mem,
                                        allow_illegal_end,
                                        &illegal_end,
                                        &step_info,
                                        error,
                                        error_size);
        if (rc != OK)
        {
            return rc;
        }

        if (illegal_end != 0u)
        {
            if (ended_on_illegal != NULL)
            {
                *ended_on_illegal = 1u;
            }

            return OK;
        }

        if (QE6502_IS_INSTR_DONE(*qe_tick))
        {
            return OK;
        }
    }

    set_error(error, error_size, "instruction did not finish within %u cycles", (unsigned)max_cycles);
    return ERR_TIMEOUT;
}

int compare_instruction_bus(qe6502_cpu_t* qe_cpu,
                            qe6502_tick_t* qe_tick,
                            uint8_t* qe_mem,
                            state_t* perfect_cpu,
                            uint8_t* perfect_mem,
                            uint32_t max_cycles,
                            char* error,
                            size_t error_size)
{
    return compare_instruction_bus_ex(qe_cpu,
                                      qe_tick,
                                      qe_mem,
                                      perfect_cpu,
                                      perfect_mem,
                                      max_cycles,
                                      0u,
                                      NULL,
                                      error,
                                      error_size);
}

int compare_final_registers(qe6502_cpu_t* qe_cpu,
                            state_t* perfect_cpu,
                            char* error,
                            size_t error_size)
{
    const uint16_t perfect_pc = readAddressBus(perfect_cpu);

    step(perfect_cpu);
    step(perfect_cpu);

    {
        const qe6502_state_t qe_state = qe6502_dump_state(qe_cpu);

        const uint16_t qe_pc = QE6502_PC(qe_state);
        const uint8_t qe_a = QE6502_A(qe_state);
        const uint8_t qe_x = QE6502_X(qe_state);
        const uint8_t qe_y = QE6502_Y(qe_state);
        const uint8_t qe_s = QE6502_S(qe_state);
        const uint8_t qe_p = normalize_p(QE6502_P(qe_state));

        const uint8_t pf_a = readA(perfect_cpu);
        const uint8_t pf_x = readX(perfect_cpu);
        const uint8_t pf_y = readY(perfect_cpu);
        const uint8_t pf_s = readSP(perfect_cpu);
        const uint8_t pf_p = normalize_p(readP(perfect_cpu));

        if ((qe_pc != perfect_pc) ||
            (qe_a != pf_a) ||
            (qe_x != pf_x) ||
            (qe_y != pf_y) ||
            (qe_s != pf_s) ||
            (qe_p != pf_p))
        {
            set_error(error,
                      error_size,
                      "final register mismatch: qe[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X] perfect[PC=%04X A=%02X X=%02X Y=%02X S=%02X P=%02X]",
                      qe_pc,
                      qe_a,
                      qe_x,
                      qe_y,
                      qe_s,
                      qe_p,
                      perfect_pc,
                      pf_a,
                      pf_x,
                      pf_y,
                      pf_s,
                      pf_p);
            return ERR_REG;
        }
    }

    return OK;
}

int compare_instruction(qe6502_cpu_t* qe_cpu,
                        qe6502_tick_t* qe_tick,
                        uint8_t* qe_mem,
                        state_t* perfect_cpu,
                        uint8_t* perfect_mem,
                        uint32_t max_cycles,
                        char* error,
                        size_t error_size)
{
    const int rc = compare_instruction_bus(qe_cpu,
                                           qe_tick,
                                           qe_mem,
                                           perfect_cpu,
                                           perfect_mem,
                                           max_cycles,
                                           error,
                                           error_size);
    if (rc != OK)
    {
        return rc;
    }

    return compare_final_registers(qe_cpu, perfect_cpu, error, error_size);
}
