#ifndef OPCODE_TEST_H
#define OPCODE_TEST_H

#include <qe6502/qe6502.h>

#include <stddef.h>
#include <stdint.h>

int run_program_test_ex(qe6502_cpu_t* cpu,
                        uint8_t* qe_mem,
                        uint8_t* perfect_mem,
                        const uint8_t* program,
                        size_t program_size,
                        uint32_t instruction_count,
                        uint8_t allow_illegal_end,
                        uint32_t* seed,
                        char* error,
                        size_t error_size);

int run_program_test(qe6502_cpu_t* cpu,
                     uint8_t* qe_mem,
                     uint8_t* perfect_mem,
                     const uint8_t* program,
                     size_t program_size,
                     uint32_t instruction_count,
                     uint32_t* seed,
                     char* error,
                     size_t error_size);

int run_opcode_test_ex(qe6502_cpu_t* cpu,
                       uint8_t* qe_mem,
                       uint8_t* perfect_mem,
                       uint8_t opcode,
                       uint8_t allow_illegal_end,
                       uint32_t* seed,
                       char* error,
                       size_t error_size);

int run_opcode_test(qe6502_cpu_t* cpu,
                    uint8_t* qe_mem,
                    uint8_t* perfect_mem,
                    uint8_t opcode,
                    uint32_t* seed,
                    char* error,
                    size_t error_size);

#endif
