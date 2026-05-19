#include "interrupt_tests.h"

static int is_nmos_model(uint8_t model)
{
    return model == (uint8_t)QE6502_MODEL_MOS || model == (uint8_t)QE6502_MODEL_NES;
}

static int is_stack_write(qe6502_tick_t tick)
{
    const uint16_t address = QE6502_ADDRESS(tick);
    return QE6502_IS_WRITING(tick) && address >= (uint16_t)0x0100u && address <= (uint16_t)0x01ffu;
}

static int step_until_stack_write(qe6502_it_context* ctx,
                                  uint8_t ordinal,
                                  uint32_t max_bus_cycles,
                                  const char** fail_message)
{
    uint8_t count = 0u;
    uint32_t steps = 0u;

    while (steps <= max_bus_cycles)
    {
        if (is_stack_write(ctx->tick))
        {
            ++count;
            if (count == ordinal)
            {
                return 1;
            }
        }

        if (!it_bus_step(ctx, fail_message))
        {
            return 0;
        }
        ++steps;
    }

    *fail_message = "nmos_quirks: did not reach requested stack write";
    return 0;
}

static int step_until_read(qe6502_it_context* ctx,
                           uint16_t address,
                           uint32_t max_bus_cycles,
                           const char** fail_message)
{
    uint32_t steps = 0u;

    while (steps <= max_bus_cycles)
    {
        if (!QE6502_IS_WRITING(ctx->tick) && QE6502_ADDRESS(ctx->tick) == address)
        {
            return 1;
        }

        if (!it_bus_step(ctx, fail_message))
        {
            return 0;
        }
        ++steps;
    }

    *fail_message = "nmos_quirks: did not reach requested read";
    return 0;
}

static const char* brk_nmi_hijack_uses_nmi_vector_and_preserves_break_bit(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;
    uint8_t saved_p;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_BRK);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    s_before = it_s(&ctx);
    IT_REQUIRE(step_until_stack_write(&ctx, 2u, 16u, &fail), fail);

    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "brk_nmi_hijack: expected NMI vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) != 0u, "brk_nmi_hijack: BRK-hijacked-by-NMI must preserve pushed B=1");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "brk_nmi_hijack: live I flag should be set after interrupt entry");

    it_set_nmi(&ctx, 0u);
    return 0;
}

static const char* brk_irq_hijack_clears_break_bit(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;
    uint8_t saved_p;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_BRK);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "brk_irq_hijack: CLI did not clear I");

    s_before = it_s(&ctx);
    IT_REQUIRE(step_until_stack_write(&ctx, 2u, 16u, &fail), fail);

    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "brk_irq_hijack: expected IRQ/BRK vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) == 0u, "brk_irq_hijack: BRK hijacked by IRQ should push B=0");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "brk_irq_hijack: live I flag should be set after interrupt entry");

    it_set_irq(&ctx, 0u);
    return 0;
}

static const char* irq_nmi_hijack_uses_nmi_vector_and_pushes_break_clear(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;
    uint8_t saved_p;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "irq_nmi_hijack: CLI did not clear I");

    s_before = it_s(&ctx);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(step_until_stack_write(&ctx, 2u, 32u, &fail), fail);

    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "irq_nmi_hijack: expected NMI vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) == 0u, "irq_nmi_hijack: IRQ hijacked by NMI should push B=0");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "irq_nmi_hijack: live I flag should be set after interrupt entry");

    it_set_irq(&ctx, 0u);
    it_set_nmi(&ctx, 0u);
    return 0;
}

static const char* lost_nmi_during_irq_acknowledge_is_not_serviced(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_NOP);
    it_write8(&ctx, (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(step_until_read(&ctx, IT_VEC_IRQ_LO, 64u, &fail), fail);

    it_set_nmi(&ctx, 1u);
    it_set_nmi(&ctx, 0u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "lost_nmi: IRQ vector should still be used");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), "lost_nmi: short NMI pulse should not be serviced after IRQ handler first instruction");

    it_set_irq(&ctx, 0u);
    return 0;
}

static const char* late_held_nmi_during_irq_vector_fetch_runs_one_irq_instruction_first(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_NOP);
    it_write8(&ctx, (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(step_until_read(&ctx, IT_VEC_IRQ_LO, 64u, &fail), fail);

    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "late_nmi: late NMI should not hijack IRQ vector");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "late_nmi: held late NMI should be serviced after one IRQ-handler instruction");

    it_set_irq(&ctx, 0u);
    it_set_nmi(&ctx, 0u);
    return 0;
}

const char* qe6502_interrupt_nmos_brk_nmi_hijack_tests(uint8_t model)
{
    return brk_nmi_hijack_uses_nmi_vector_and_preserves_break_bit(model);
}

const char* qe6502_interrupt_nmos_brk_irq_hijack_tests(uint8_t model)
{
    return brk_irq_hijack_clears_break_bit(model);
}

const char* qe6502_interrupt_nmos_irq_nmi_hijack_tests(uint8_t model)
{
    return irq_nmi_hijack_uses_nmi_vector_and_pushes_break_clear(model);
}

const char* qe6502_interrupt_nmos_lost_nmi_tests(uint8_t model)
{
    return lost_nmi_during_irq_acknowledge_is_not_serviced(model);
}

const char* qe6502_interrupt_nmos_late_nmi_tests(uint8_t model)
{
    return late_held_nmi_during_irq_vector_fetch_runs_one_irq_instruction_first(model);
}

const char* qe6502_interrupt_nmos_quirk_tests(uint8_t model)
{
    const char* fail;

    fail = qe6502_interrupt_nmos_brk_nmi_hijack_tests(model);
    if (fail) return fail;

    fail = qe6502_interrupt_nmos_brk_irq_hijack_tests(model);
    if (fail) return fail;

    fail = qe6502_interrupt_nmos_irq_nmi_hijack_tests(model);
    if (fail) return fail;

    fail = qe6502_interrupt_nmos_lost_nmi_tests(model);
    if (fail) return fail;

    fail = qe6502_interrupt_nmos_late_nmi_tests(model);
    if (fail) return fail;

    return 0;
}
