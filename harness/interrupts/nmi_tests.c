#include "interrupt_tests.h"

static const char* nmi_pulse_latched_mid_instruction(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_LDA_ABS);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 1u), (uint8_t)0x34u);
    it_write8(&ctx, (uint16_t)(IT_ADDR_MAIN + 2u), (uint8_t)0x12u);
    it_write8(&ctx, (uint16_t)0x1234u, (uint8_t)0x42u);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);

    IT_REQUIRE(it_boot(&ctx, &fail), fail);
    IT_REQUIRE(it_bus_step(&ctx, &fail), fail);
    IT_REQUIRE(!QE6502_IS_INSTR_DONE(ctx.tick), "nmi_mid: expected to be inside LDA before pulsing NMI");

    it_set_nmi(&ctx, 1u);
    it_set_nmi(&ctx, 0u);

    IT_REQUIRE(it_step_until_boundary(&ctx, 64u, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "nmi_mid: NMI pulse was not latched");
    IT_REQUIRE(it_a(&ctx) == (uint8_t)0x42u, "nmi_mid: interrupted before completing current instruction");
    return 0;
}

static const char* nmi_takes_even_when_i_set(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;
    uint8_t s_before;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_NOP);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);
    IT_REQUIRE((it_p(&ctx) & IT_FLAG_I) != 0u, "nmi_i: reset should leave I set");

    s_before = it_s(&ctx);
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "nmi_i: NMI should be taken even when I is set");
    IT_REQUIRE((it_read8(&ctx, it_stack_addr((uint8_t)(s_before - 2u))) & IT_FLAG_B) == 0u, "nmi_i: hardware NMI should push B=0");
    return 0;
}

static const char* nmi_low_level_does_not_retrigger(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "nmi_low: first NMI not taken");

    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 1u), "nmi_low: NMI retriggered while pin remained low");
    return 0;
}

static const char* nmi_retriggers_after_high_to_low_again(uint8_t model)
{
    const char* fail = 0;
    qe6502_it_context ctx;

    it_init(&ctx, model);
    it_write8(&ctx, IT_ADDR_MAIN, IT_OP_NOP);
    it_write8(&ctx, IT_ADDR_NMI_HANDLER, IT_OP_RTI);
    IT_REQUIRE(it_boot(&ctx, &fail), fail);

    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "nmi_retrigger: first NMI not taken");
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == (uint16_t)(IT_ADDR_MAIN + 1u), "nmi_retrigger: first RTI did not return to main");

    it_set_nmi(&ctx, 0u);
    it_set_nmi(&ctx, 1u);
    IT_REQUIRE(it_step_instruction(&ctx, &fail), fail);
    IT_REQUIRE(it_pc(&ctx) == IT_ADDR_NMI_HANDLER, "nmi_retrigger: second high-to-low transition did not retrigger NMI");
    return 0;
}

const char* qe6502_interrupt_nmi_tests(uint8_t model)
{
    const char* fail;

    fail = nmi_pulse_latched_mid_instruction(model);
    if (fail) return fail;

    fail = nmi_takes_even_when_i_set(model);
    if (fail) return fail;

    fail = nmi_low_level_does_not_retrigger(model);
    if (fail) return fail;

    fail = nmi_retriggers_after_high_to_low_again(model);
    if (fail) return fail;

    return 0;
}
