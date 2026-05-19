#ifndef QE6502_INTERRUPT_TESTS_H
#define QE6502_INTERRUPT_TESTS_H

#include <qe6502/qe6502.h>
#include <stdint.h>
#include <stddef.h>

#define IT_MEM_SIZE 65536u

#define IT_ADDR_MAIN        ((uint16_t)0x8000u)
#define IT_ADDR_NMI_HANDLER ((uint16_t)0x9000u)
#define IT_ADDR_IRQ_HANDLER ((uint16_t)0xA000u)

#define IT_VEC_NMI_LO       ((uint16_t)0xFFFAu)
#define IT_VEC_NMI_HI       ((uint16_t)0xFFFBu)
#define IT_VEC_RESET_LO     ((uint16_t)0xFFFCu)
#define IT_VEC_RESET_HI     ((uint16_t)0xFFFDu)
#define IT_VEC_IRQ_LO       ((uint16_t)0xFFFEu)
#define IT_VEC_IRQ_HI       ((uint16_t)0xFFFFu)

#define IT_OP_BRK           ((uint8_t)0x00u)
#define IT_OP_RTI           ((uint8_t)0x40u)
#define IT_OP_CLI           ((uint8_t)0x58u)
#define IT_OP_SEI           ((uint8_t)0x78u)
#define IT_OP_LDA_ABS       ((uint8_t)0xADu)
#define IT_OP_NOP           ((uint8_t)0xEAu)

#define IT_FLAG_I           ((uint8_t)0x04u)

#define IT_REQUIRE(cond, message) do { if (!(cond)) { return (message); } } while (0)

typedef struct qe6502_it_context
{
    qe6502_cpu_t cpu;
    qe6502_tick_t tick;
    uint8_t memory[IT_MEM_SIZE];
    uint64_t bus_cycles;
    uint8_t model;
} qe6502_it_context;

void it_init(qe6502_it_context* ctx, uint8_t model);
void it_write8(qe6502_it_context* ctx, uint16_t address, uint8_t value);
uint8_t it_read8(const qe6502_it_context* ctx, uint16_t address);
int it_boot(qe6502_it_context* ctx, const char** fail_message);
int it_bus_step(qe6502_it_context* ctx, const char** fail_message);
int it_step_until_boundary(qe6502_it_context* ctx, uint32_t max_bus_cycles, const char** fail_message);
int it_step_instruction(qe6502_it_context* ctx, const char** fail_message);
void it_set_irq(qe6502_it_context* ctx, uint8_t low);
void it_set_nmi(qe6502_it_context* ctx, uint8_t low);
uint16_t it_pc(const qe6502_it_context* ctx);
uint8_t it_a(const qe6502_it_context* ctx);
uint8_t it_p(const qe6502_it_context* ctx);
uint8_t it_s(const qe6502_it_context* ctx);
uint16_t it_stack_addr(uint8_t s);

const char* qe6502_interrupt_irq_tests(uint8_t model);
const char* qe6502_interrupt_nmi_tests(uint8_t model);
const char* qe6502_interrupt_combined_tests(uint8_t model);

#endif /* QE6502_INTERRUPT_TESTS_H */
