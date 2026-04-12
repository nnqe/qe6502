/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE6502_H__
#define QE6502_H__

#include "qe6502_macros.h"
#include <stdint.h>
#include <stddef.h>

#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
#define QE6502_MODEL_MOS 0
#define QE6502_MODEL_NES 1
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
#define QE6502_MODEL_WDC 2
#define QE6502_MODEL_RW  3
#define QE6502_MODEL_ST  4
#endif

QE_FFI_API(uint32_t)
qe6502_version(void);
//

QE_FFI_API(size_t)
qe6502_cpu_size(void);

//

QE_FFI_API(size_t)
qe6502_cpu_create(void* cpu_memory, size_t memory_size);

QE_FFI_API(void)
qe6502_cpu_power_on(void* cpu, uint8_t model);

QE_FFI_API(void)
qe6502_cpu_tick(void* cpu);

//

QE_FFI_API(uint8_t)  qe6502_ok(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_needs_data(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_has_data(const void* cpu);
QE_FFI_API(void)     qe6502_feed_data(void* cpu, uint8_t byte);
QE_FFI_API(uint8_t)  qe6502_data(const void* cpu);
QE_FFI_API(uint16_t) qe6502_address(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_instr_done(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_started(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_model(const void* cpu);

QE_FFI_API(uint8_t)  qe6502_nmi_pin(const void* cpu);
QE_FFI_API(void)     qe6502_nmi_hi(void* cpu);
QE_FFI_API(void)     qe6502_nmi_lo(void* cpu);

QE_FFI_API(uint8_t)  qe6502_irq_pin(const void* cpu);
QE_FFI_API(void)     qe6502_irq_hi(void* cpu);
QE_FFI_API(void)     qe6502_irq_lo(void* cpu);


typedef struct
{
    uint8_t word_lsb;
    uint8_t word_msb;
    uint8_t word32_lsw;
    uint8_t word32_msw;
    uint8_t cmd_address;
    uint8_t cmd_offset;
    uint8_t cmd_flags;
    uint8_t cmd_packed;
    uint8_t address;
    uint8_t pointer;
    uint8_t error_code;
    uint8_t instr;
    uint8_t merged;
    uint8_t data;
    uint8_t pagecross_overflow;
    uint8_t model;
    uint8_t istate;
    uint8_t opcode;
    uint8_t PC;
    uint8_t S;
    uint8_t A;
    uint8_t X;
    uint8_t Y;
    uint8_t P;
    //
    uint8_t sizeof_word;
    uint8_t sizeof_word32;
    uint8_t sizeof_cmd_address;
    uint8_t sizeof_cmd_offset;
    uint8_t sizeof_cmd_flags;
    uint8_t sizeof_cmd_packed;
    uint8_t sizeof_address;
    uint8_t sizeof_pointer;
    uint8_t sizeof_error_code;
    uint8_t sizeof_instr;
    uint8_t sizeof_merged;
    uint8_t sizeof_data;
    uint8_t sizeof_pagecross_overflow;
    uint8_t sizeof_model;
    uint8_t sizeof_istate;
    uint8_t sizeof_opcode;
    uint8_t sizeof_PC;
    uint8_t sizeof_S;
    uint8_t sizeof_A;
    uint8_t sizeof_X;
    uint8_t sizeof_Y;
    uint8_t sizeof_P;
} qe6502_offsets_t;

QE_FFI_API(void)
qe6502_offsets(qe6502_offsets_t* offsets);


#endif // QE6502_H__
