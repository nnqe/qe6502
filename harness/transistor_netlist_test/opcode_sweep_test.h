#ifndef OPCODE_SWEEP_TEST_H
#define OPCODE_SWEEP_TEST_H

#include <qe6502/qe6502.h>

#include <stddef.h>
#include <stdint.h>

int run_opcode_sweep_test(qe6502_cpu_t* cpu,
                          uint8_t* qe_mem,
                          uint8_t* perfect_mem,
                          uint32_t* seed,
                          uint32_t trials_per_opcode,
                          uint8_t include_illegal,
                          char* error,
                          size_t error_size);

#endif
