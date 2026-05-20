#ifndef NETLIST_HELPERS_H
#define NETLIST_HELPERS_H

#include <qe6502/qe6502.h>
#include "third_party/perfect6502/perfect6502.h"

#include <stddef.h>
#include <stdint.h>

enum
{
    OK = 0,
    ERR_ARG = 1,
    ERR_INIT = 2,
    ERR_TIMEOUT = 3,
    ERR_BUS = 4,
    ERR_REG = 5,
    ERR_CPU = 6
};

typedef struct qe_step_info_t
{
    qe6502_tick_t before;
    qe6502_tick_t after;
    uint16_t address;
    uint8_t data;
    uint8_t write;
    uint8_t illegal;
    uint16_t error_code;
} qe_step_info_t;

uint32_t rnd_next(uint32_t* seed);

int fill_random_memory(uint8_t* qe_mem,
                       uint8_t* perfect_mem,
                       uint32_t* seed);

int randomize_vectors(uint8_t* qe_mem,
                      uint8_t* perfect_mem,
                      uint32_t* seed,
                      uint16_t* reset_pc);

int init_perfect_to_fetch(state_t** out_cpu,
                          uint16_t* out_pc,
                          unsigned long max_half_cycles,
                          char* error,
                          size_t error_size);

int init_qe_to_fetch(qe6502_cpu_t* cpu,
                     qe6502_tick_t* tick,
                     uint8_t* qe_mem,
                     uint16_t fetch_pc,
                     uint32_t max_cycles,
                     char* error,
                     size_t error_size);

int compare_cycle(qe6502_cpu_t* qe_cpu,
                  qe6502_tick_t* qe_tick,
                  uint8_t* qe_mem,
                  state_t* perfect_cpu,
                  uint8_t* perfect_mem,
                  char* error,
                  size_t error_size);

int compare_cycle_ex(qe6502_cpu_t* qe_cpu,
                     qe6502_tick_t* qe_tick,
                     uint8_t* qe_mem,
                     state_t* perfect_cpu,
                     uint8_t* perfect_mem,
                     uint8_t allow_illegal_end,
                     uint8_t* ended_on_illegal,
                     qe_step_info_t* step_info,
                     char* error,
                     size_t error_size);

int compare_instruction_bus(qe6502_cpu_t* qe_cpu,
                            qe6502_tick_t* qe_tick,
                            uint8_t* qe_mem,
                            state_t* perfect_cpu,
                            uint8_t* perfect_mem,
                            uint32_t max_cycles,
                            char* error,
                            size_t error_size);

int compare_instruction_bus_ex(qe6502_cpu_t* qe_cpu,
                               qe6502_tick_t* qe_tick,
                               uint8_t* qe_mem,
                               state_t* perfect_cpu,
                               uint8_t* perfect_mem,
                               uint32_t max_cycles,
                               uint8_t allow_illegal_end,
                               uint8_t* ended_on_illegal,
                               char* error,
                               size_t error_size);

int compare_final_registers(qe6502_cpu_t* qe_cpu,
                            state_t* perfect_cpu,
                            char* error,
                            size_t error_size);

int compare_instruction(qe6502_cpu_t* qe_cpu,
                        qe6502_tick_t* qe_tick,
                        uint8_t* qe_mem,
                        state_t* perfect_cpu,
                        uint8_t* perfect_mem,
                        uint32_t max_cycles,
                        char* error,
                        size_t error_size);

#endif
