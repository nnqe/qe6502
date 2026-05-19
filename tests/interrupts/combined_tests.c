#include "interrupt_tests.h"

static const char* nmi_priority_over_irq_then_irq_after_rti(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) == 0u, "combo_priority: CLI did not clear I");

    it_set_irq(&ctx, 1u);
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "combo_priority: NMI should have priority over IRQ");

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "combo_priority: IRQ should be taken after NMI RTI while IRQ remains low");

    it_set_irq(&ctx, 0u);
    it_set_nmi(&ctx, 0u);
    return 0;
}

static const char* nmi_can_interrupt_irq_handler(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_CLI);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_IRQ_HANDLER, IT_OP_NOP);
    it_write8(&ctx, (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), IT_OP_RTI);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    it_set_irq(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_IRQ_HANDLER, "combo_nested: IRQ not taken");
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "combo_nested: IRQ handler should run with I set");

    IT_REQUIRE(it_bus_step(&ctx, &fail), fail);
    IT_REQUIRE(!QE6502_IS_INSTR_DONE(ctx.tick), "combo_nested: expected to be inside IRQ-handler NOP");
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "combo_nested: NMI did not interrupt IRQ handler");

    it_set_nmi(&ctx, 0u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_IRQ_HANDLER + 1u), "combo_nested: NMI RTI did not return after IRQ-handler NOP");

    it_set_irq(&ctx, 0u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 2u), "combo_nested: IRQ RTI did not return to main PC");
    return 0;
}

const char* qe6502_interrupt_combined_tests(uint8_t model)
{
    const char* fail;

    fail = nmi_priority_over_irq_then_irq_after_rti(model);
    if (fail) return fail;

    fail = nmi_can_interrupt_irq_handler(model);
    if (fail) return fail;

    return 0;
}
