#include "interrupt_tests.h"

static const char* irq_level_accepted_at_boundary(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_MAIN, "irq_level: expected reset PC");

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 1u), "irq_level: CLI did not advance PC");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "irq_level: CLI did not clear I flag");

    s_before = it_s(&ctx);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "irq_level: IRQ vector was not taken");
    IT_REQUIRE(it_s(&ctx) == (uint8_t)(s_before - 3u), "irq_level: IRQ did not push three bytes");
    IT_REQUIRE(it_read8(&ctx, it_stack_addr(s_before)) == (uint8_t)(IT_ADDR_MAIN >> 8), "irq_level: wrong pushed PCH");
    IT_REQUIRE(it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 1u))) == (uint8_t)((IT_ADDR_MAIN + 2u) & 0x00ffu), "irq_level: wrong pushed PCL");
    IT_REQUIRE((it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u))) & IT_FLAG_I) == 0u, "irq_level: pushed P should preserve old I=0");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "irq_level: IRQ handler should run with I set");

    return 0;
}

static const char* irq_short_pulse_is_missed(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 2u), IT_OP_NOP);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    s_before = it_s(&ctx);

    it_set_irq(&ctx, 1u);
    it_set_irq(&ctx, 0u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 2u), "irq_pulse: short IRQ pulse should be missed");
    IT_REQUIRE(it_s(&ctx) == s_before, "irq_pulse: stack changed although IRQ was missed");
    return 0;
}

static const char* irq_masked_by_i_flag(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_NOP);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "irq_masked: reset should leave I set");

    s_before = it_s(&ctx);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);

    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 1u), "irq_masked: IRQ should not interrupt while I is set");
    IT_REQUIRE(it_s(&ctx) == s_before, "irq_masked: stack changed while IRQ was masked");
    it_set_irq(&ctx, 0u);
    return 0;
}

static const char* irq_reenters_after_rti_if_level_still_low(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "irq_reenter: first IRQ not taken");

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "irq_reenter: IRQ should re-enter after RTI while level remains low");

    it_set_irq(&ctx, 0u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 2u), "irq_reenter: RTI after deassert should return to main PC");
    return 0;
}

const char* qe6502_interrupt_irq_tests(uint8_t model)
{
    const char* fail;

    fail = irq_level_accepted_at_boundary(model);
    if (fail) return fail;

    fail = irq_short_pulse_is_missed(model);
    if (fail) return fail;

    fail = irq_masked_by_i_flag(model);
    if (fail) return fail;

    fail = irq_reenters_after_rti_if_level_still_low(model);
    if (fail) return fail;

    return 0;
}
