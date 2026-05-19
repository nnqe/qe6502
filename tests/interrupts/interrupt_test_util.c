#include "interrupt_tests.h"
#include <string.h>

static void it_set_vector(qe6502_it_context* ctx, uint16_t vector_address, uint16_t target)
{
    ctx->memory[vector_address] = (uint8_t)(target & 0x00ffu);
    ctx->memory[(uint16_t)(vector_address + 1u)] = (uint8_t)((target >> 8) & 0x00ffu);
}

void it_init(qe6502_it_context* ctx, uint8_t model)
{
    size_t i;
    memset(ctx, 0, sizeof(*ctx));
    ctx->model = model;
    for (i = 0u; i < sizeof(ctx->memory); ++i)
    {
        ctx->memory[i] = IT_OP_NOP;
    }

    it_set_vector(ctx, IT_VEC_RESET_LO, IT_ADDR_MAIN);
    it_set_vector(ctx, IT_VEC_NMI_LO, IT_ADDR_NMI_HANDLER);
    it_set_vector(ctx, IT_VEC_IRQ_LO, IT_ADDR_IRQ_HANDLER);

    ctx->memory[IT_ADDR_NMI_HANDLER] = IT_OP_RTI;
    ctx->memory[IT_ADDR_IRQ_HANDLER] = IT_OP_RTI;
}

void it_write8(qe6502_it_context* ctx, uint16_t address, uint8_t value)
{
    ctx->memory[address] = value;
}

uint8_t it_read8(const qe6502_it_context* ctx, uint16_t address)
{
    return ctx->memory[address];
}

int it_boot(qe6502_it_context* ctx, const char** fail_message)
{
    uint32_t guard = 0u;
    ctx->tick = qe6502_reset(&ctx->cpu, ctx->model);
    if (QE6502_IS_NOT_OK(ctx->tick))
    {
        *fail_message = "reset returned an error tick";
        return 0;
    }

    while (QE6502_IS_STARTING(ctx->tick))
    {
        if (!it_bus_step(ctx, fail_message))
        {
            return 0;
        }
        ++guard;
        if (guard > 32u)
        {
            *fail_message = "reset did not finish within 32 bus cycles";
            return 0;
        }
    }

    if (!QE6502_IS_INSTR_DONE(ctx->tick))
    {
        *fail_message = "reset did not stop at an opcode-fetch boundary";
        return 0;
    }
    return 1;
}

int it_bus_step(qe6502_it_context* ctx, const char** fail_message)
{
    const uint16_t address = QE6502_ADDRESS(ctx->tick);
    uint8_t data = 0u;

    if (QE6502_IS_NOT_OK(ctx->tick))
    {
        *fail_message = qe6502_error_string(&ctx->cpu);
        return 0;
    }

    if (QE6502_IS_WRITING(ctx->tick))
    {
        data = QE6502_DATA(ctx->tick);
        ctx->memory[address] = data;
    }
    else
    {
        data = ctx->memory[address];
    }

    ctx->tick = qe6502_tick(&ctx->cpu, data);
    ctx->bus_cycles += 1u;

    if (QE6502_IS_NOT_OK(ctx->tick))
    {
        *fail_message = qe6502_error_string(&ctx->cpu);
        return 0;
    }

    return 1;
}

int it_step_until_boundary(qe6502_it_context* ctx, uint32_t max_bus_cycles, const char** fail_message)
{
    uint32_t steps = 0u;
    while (!QE6502_IS_INSTR_DONE(ctx->tick))
    {
        if (!it_bus_step(ctx, fail_message))
        {
            return 0;
        }
        ++steps;
        if (steps > max_bus_cycles)
        {
            *fail_message = "instruction did not reach boundary within max bus cycles";
            return 0;
        }
    }
    return 1;
}

int it_step_instruction(qe6502_it_context* ctx, const char** fail_message)
{
    uint32_t steps = 0u;

    do
    {
        if (!it_bus_step(ctx, fail_message))
        {
            return 0;
        }
        ++steps;
        if (steps > 64u)
        {
            *fail_message = "single instruction/interrupt exceeded 64 bus cycles";
            return 0;
        }
    } while (!QE6502_IS_INSTR_DONE(ctx->tick));

    return 1;
}

void it_set_irq(qe6502_it_context* ctx, uint8_t low)
{
    ctx->tick = qe6502_set_irq(&ctx->cpu, low);
}

void it_set_nmi(qe6502_it_context* ctx, uint8_t low)
{
    ctx->tick = qe6502_set_nmi(&ctx->cpu, low);
}

uint16_t it_pc(const qe6502_it_context* ctx)
{
    return QE6502_PC(qe6502_dump_state(&ctx->cpu));
}

uint8_t it_a(const qe6502_it_context* ctx)
{
    return QE6502_A(qe6502_dump_state(&ctx->cpu));
}

uint8_t it_p(const qe6502_it_context* ctx)
{
    return QE6502_P(qe6502_dump_state(&ctx->cpu));
}

uint8_t it_s(const qe6502_it_context* ctx)
{
    return QE6502_S(qe6502_dump_state(&ctx->cpu));
}

uint16_t it_stack_addr(uint8_t s)
{
    return (uint16_t)(0x0100u | (uint16_t)s);
}
