#ifndef INTERRUPT_TEST_H
#define INTERRUPT_TEST_H

#include <qe6502/qe6502.h>

#include <stddef.h>
#include <stdint.h>

int run_irq_lda_abs_timing_test(qe6502_cpu_t* cpu,
                                uint8_t* qe_mem,
                                uint8_t* perfect_mem,
                                uint32_t* seed,
                                char* error,
                                size_t error_size);

#endif
