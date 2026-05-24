#ifndef SETUP_TEST_H
#define SETUP_TEST_H

#include <qe6502/qe6502.h>
#include "third_party/perfect6502/perfect6502.h"

#include <stddef.h>
#include <stdint.h>

enum
{
    SETUP_DATA_DONE = 0u,
    SETUP_DATA_A = 1u,
    SETUP_DATA_X = 2u,
    SETUP_DATA_Y = 3u,
    SETUP_DATA_P = 4u,
    SETUP_DATA_S = 5u,
    SETUP_DATA_PCL = 6u,
    SETUP_DATA_PCH = 7u,
    SETUP_DATA_RTI_SP = 8u,
    SETUP_DATA_SIZE = 9u
};


#define SETUP_CODE_ADDR 0xff10u
#define SETUP_CODE_END  0xff80u
#define SETUP_DATA_ADDR 0xff00u

typedef struct regs_t
{
    uint16_t pc;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t s;
} regs_t;

void randomize_regs(regs_t* regs, uint32_t* seed);

int inject_setup_program(uint8_t* mem,
                         uint16_t code_addr,
                         uint16_t data_addr,
                         const regs_t* regs);

int inject_setup_program_both(uint8_t* qe_mem,
                              uint8_t* perfect_mem,
                              uint16_t code_addr,
                              uint16_t data_addr,
                              const regs_t* regs);

int run_setup_qe(qe6502_cpu_t* cpu,
                 uint8_t* qe_mem,
                 uint16_t code_addr,
                 uint16_t data_addr,
                 const regs_t* regs,
                 char* error,
                 size_t error_size);

int run_setup_qe_fetch(qe6502_cpu_t* cpu,
                       uint8_t* qe_mem,
                       uint16_t code_addr,
                       uint16_t data_addr,
                       const regs_t* regs,
                       qe6502_tick_t* out_tick,
                       char* error,
                       size_t error_size);

int run_setup_perfect(state_t** out_cpu,
                      uint8_t* perfect_mem,
                      uint16_t code_addr,
                      uint16_t data_addr,
                      const regs_t* regs,
                      char* error,
                      size_t error_size);

int run_setup_pair_test(qe6502_cpu_t* qe_cpu,
                        uint8_t* qe_mem,
                        uint8_t* perfect_mem,
                        uint16_t code_addr,
                        uint16_t data_addr,
                        const regs_t* regs,
                        char* error,
                        size_t error_size);

int run_random_setup_tests(qe6502_cpu_t* qe_cpu,
                           uint8_t* qe_mem,
                           uint8_t* perfect_mem,
                           uint32_t* seed,
                           uint32_t count,
                           char* error,
                           size_t error_size);

#endif
