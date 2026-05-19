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

static const char* brk_nmi_hijack_t4_uses_nmi_vector_and_preserves_break_bit(uint8_t model)
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

    /*
     * External source: NMOS NMI can hijack an IRQ/BRK vector if NMI becomes
     * active no later than the end of T4 of the BRK-like sequence.
     *
     * qe6502 author interpretation: when this happens during the stack-write
     * window modeled here as T3/T4, the BRK-style pushed B value has already
     * been selected, so the NMI vector is used but saved P keeps B=1.
     */
    IT_REQUIRE(step_until_stack_write(&ctx, 2u, 16u, &fail), fail);
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "brk_nmi_hijack_t4: expected NMI vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) != 0u, "brk_nmi_hijack_t4: T3/T4 BRK hijack should preserve pushed B=1");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "brk_nmi_hijack_t4: live I flag should be set after interrupt entry");

    it_set_nmi(&ctx, 0u);
    return 0;
}

static const char* irq_before_brk_status_push_clears_break_bit(uint8_t model)
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
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "irq_before_brk: CLI did not clear I");

    s_before = it_s(&ctx);

    /*
     * External source: the BRK/B-bit machinery is split; an IRQ pending before
     * the BRK status-push decision can make the pushed B value zero.
     * This test asserts IRQ during BRK's dummy-read phase, before qe6502's
     * T3/T4 stack-write hijack window interpretation.
     */
    IT_REQUIRE(step_until_read(&ctx, (uint16_t)(IT_ADDR_MAIN + 2u), 16u, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "irq_before_brk: expected IRQ/BRK vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) == 0u, "irq_before_brk: IRQ before status decision should push B=0");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "irq_before_brk: live I flag should be set after interrupt entry");

    it_set_irq(&ctx, 0u);
    return 0;
}

static const char* brk_irq_hijack_t4_preserves_break_bit(uint8_t model)
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
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "brk_irq_hijack_t4: CLI did not clear I");

    s_before = it_s(&ctx);

    /*
     * qe6502 author interpretation: if IRQ appears in the modeled T3/T4
     * BRK-like stack-write window, it can still select the IRQ/BRK vector but
     * the BRK-style pushed B value has already been selected. This exact cycle
     * split is an implementation interpretation of the Visual6502 behavior,
     * not a directly specified external timing guarantee.
     */
    IT_REQUIRE(step_until_stack_write(&ctx, 2u, 16u, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    saved_p = it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u)));
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "brk_irq_hijack_t4: expected IRQ/BRK vector");
    IT_REQUIRE((saved_p & IT_FLAG_B) != 0u, "brk_irq_hijack_t4: T3/T4 BRK hijack should preserve pushed B=1");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "brk_irq_hijack_t4: live I flag should be set after interrupt entry");

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

    /*
     * External source: NMI can hijack an IRQ BRK-like sequence and select the
     * NMI vector if it appears by the T4/T5 boundary. Since the sequence source
     * is hardware IRQ, the pushed B value remains IRQ-style B=0.
     */
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

    /*
     * External source: Visual6502 documents a lost NMI when a short NMI pulse
     * occurs during IRQ acknowledge shortly before IRQ vector fetch, remains
     * low for about 2.5 cycles, and is released before the later recognition
     * point. The NMI is never serviced.
     */
    it_set_nmi(&ctx, 1u);
    it_set_nmi(&ctx, 0u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "lost_nmi: IRQ vector should still be used");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), "lost_nmi: short NMI pulse should not be serviced after IRQ handler first instruction");

    it_set_irq(&ctx, 0u);
    return 0;
}

static const char* later_held_nmi_during_irq_vector_fetch_runs_one_irq_instruction_first(uint8_t model)
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

    /*
     * External source: if NMI is too late to hijack IRQ/BRK vector fetch but
     * remains asserted, the first instruction of the IRQ/BRK handler executes
     * before the NMI is serviced. This also verifies that no half-vector hijack
     * occurs in this window.
     */
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "later_nmi_irq: later NMI should not hijack IRQ vector");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "later_nmi_irq: held later NMI should be serviced after one IRQ-handler instruction");

    it_set_irq(&ctx, 0u);
    it_set_nmi(&ctx, 0u);
    return 0;
}

static const char* later_held_nmi_during_brk_vector_fetch_runs_one_brk_instruction_first(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    if (!is_nmos_model(model))
    {
        return 0;
    }

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_BRK);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_NOP);
    it_write8(&ctx, (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(step_until_read(&ctx, IT_VEC_IRQ_LO, 64u, &fail), fail);

    /*
     * External source: the same NMI half-hijack protection is described for
     * IRQ/BRK vector fetch. qe6502 author interpretation: this BRK variant is
     * the software-BRK counterpart of the later IRQ test above.
     */
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "later_nmi_brk: later NMI should not hijack BRK vector");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "later_nmi_brk: held later NMI should be serviced after one BRK-handler instruction");

    it_set_nmi(&ctx, 0u);
    return 0;
}

const char* qe6502_interrupt_nmos_brk_nmi_hijack_tests(uint8_t model)
{
    return brk_nmi_hijack_t4_uses_nmi_vector_and_preserves_break_bit(model);
}

const char* qe6502_interrupt_nmos_brk_irq_hijack_tests(uint8_t model)
{
    const char* fail;

    fail = irq_before_brk_status_push_clears_break_bit(model);
    if (fail) return fail;

    fail = brk_irq_hijack_t4_preserves_break_bit(model);
    if (fail) return fail;

    return 0;
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
    const char* fail;

    fail = later_held_nmi_during_irq_vector_fetch_runs_one_irq_instruction_first(model);
    if (fail) return fail;

    fail = later_held_nmi_during_brk_vector_fetch_runs_one_brk_instruction_first(model);
    if (fail) return fail;

    return 0;
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
