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

#include <qe/api_public.h>

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

#define QE_CONTEXT_SIZE   40u
#define QE_CONTEXT_ALIGN  8u

typedef struct qe6502_cpu_t
{
    QE_ALIGNAS(QE_CONTEXT_ALIGN)
    unsigned char bytes[QE_CONTEXT_SIZE];
} qe6502_cpu_t;

typedef enum
{
    QE6502_ADDR_MODE_ILLEGAL        = 0,
    QE6502_ADDR_MODE_ABSOLUTE       = 1,
    QE6502_ADDR_MODE_ABSOLUTE_X     = 2,
    QE6502_ADDR_MODE_ABSOLUTE_Y     = 3,
    QE6502_ADDR_MODE_ACCUMULATOR    = 4,
    QE6502_ADDR_MODE_IDX_INDIRECT_X = 5,
    QE6502_ADDR_MODE_IMMEDIATE      = 6,
    QE6502_ADDR_MODE_IMPLIED        = 7,
    QE6502_ADDR_MODE_INDIRECT       = 8,
    QE6502_ADDR_MODE_INDIRECT_IDX_Y = 9,
    QE6502_ADDR_MODE_INDIRECT_ZP    = 10,
    QE6502_ADDR_MODE_RELATIVE       = 11,
    QE6502_ADDR_MODE_ZP             = 12,
    QE6502_ADDR_MODE_ZP_RELATIVE    = 13,
    QE6502_ADDR_MODE_ZP_X           = 14,
    QE6502_ADDR_MODE_ZP_Y           = 15,
    QE6502_ADDR_MODE_ABSOLUTE_X_INDIRECT = 16
} qe6502_addr_mode_t;

typedef struct
{
    const char* name;
    const char* description;
    const char* addr_mode_str;
    const void* reserved_ptr;
    uint8_t opcode;
    uint8_t bytes;
    uint8_t addr_mode;
    uint8_t is_cmos_extension;
    uint8_t reserved_data[4];
} qe6502_opcode_meta_t;

QE_FFI_API(uint32_t)    qe6502_version(void);
//
QE_FFI_API(size_t)      qe6502_cpu_size(void);
QE_FFI_API(size_t)      qe6502_cpu_align(void);
//
QE_FFI_API(void)        qe6502_cpu_power_on(qe6502_cpu_t* cpu, uint8_t model);
QE_FFI_API(void)        qe6502_cpu_tick(qe6502_cpu_t* cpu);

/*
 * Returns a packed CPU state and BUS operation *
 * Bit layout:
 *
 *   bits [ 0..15]  : Memory address
 *   bits [16]      : Bus direction, 0 == Read request, 1 == Write request
 *   bits [17]      : 0 == Started | 1 == Starting
 *   bits [18]      : 0 == During instruction | 1 == Instruction done
 *   bits [19..22]  : Reserved
 *   bits [23]      : 0 == OK | 1 == Halted / not OK
 *   bits [24..31]  : Data out, valid only when bit [16] == 1
 *
 * This is a numeric bit encoding. Decode with shifts and masks.
 */
QE_FFI_API(uint32_t)    qe6502_cpu_tick_ex(qe6502_cpu_t* cpu, uint8_t data_in);

//
QE_FFI_API(uint8_t)     qe6502_ok(const qe6502_cpu_t* cpu);
QE_FFI_API(uint8_t)     qe6502_needs_data(const qe6502_cpu_t* cpu);
QE_FFI_API(uint8_t)     qe6502_has_data(const qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_feed_data(qe6502_cpu_t* cpu, uint8_t byte);
QE_FFI_API(uint8_t)     qe6502_read_data(const qe6502_cpu_t* cpu);
QE_FFI_API(uint16_t)    qe6502_address(const qe6502_cpu_t* cpu);
QE_FFI_API(uint8_t)     qe6502_is_instr_done(const qe6502_cpu_t* cpu);
QE_FFI_API(uint8_t)     qe6502_is_started(const qe6502_cpu_t* cpu);
QE_FFI_API(uint8_t)     qe6502_model(const qe6502_cpu_t* cpu);

QE_FFI_API(uint8_t)     qe6502_read_nmi_pin(const qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_nmi_hi(qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_nmi_lo(qe6502_cpu_t* cpu);

QE_FFI_API(uint8_t)     qe6502_read_irq_pin(const qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_irq_hi(qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_irq_lo(qe6502_cpu_t* cpu);

/*
 * Returns a packed snapshot of the visible CPU registers.
 *
 * Bit layout:
 *   bits [ 0.. 7] : PC LSB
 *   bits [ 8..15] : PC MSB
 *   bits [16..23] : A
 *   bits [24..31] : X
 *   bits [32..39] : Y
 *   bits [40..47] : S
 *   bits [48..55] : P
 *   bits [56..58] : CPU Model
 *   bits [59    ] : 0 == Ok, Reserved - used internally
 *   bits [60    ] : 0 == NMI pin low | 1 == NMI pin high
 *   bits [61    ] : 0 == IRQ pin low | 1 == IRQ pin high
 *   bits [62    ] : 0 == Running | 1 == Waiting (WAI)
 *   bits [63    ] : 0 == CPU is stabel/serializable | 1 == CPU not stable/not serializable/stopped
 *
 * This is a numeric bit encoding. Decode with shifts and masks.
 */
QE_FFI_API(uint64_t)    qe6502_dump(const qe6502_cpu_t* cpu);
QE_FFI_API(void)        qe6502_recover(qe6502_cpu_t* cpu, uint64_t stable_state);

QE_FFI_API(uint16_t)    qe6502_error_code(const qe6502_cpu_t* cpu);
QE_FFI_API(const char*) qe6502_error_string(const qe6502_cpu_t* cpu);
QE_FFI_API(const char*) qe6502_decode_error(uint16_t error_code);

QE_FFI_API(const qe6502_opcode_meta_t*) qe6502_opcode_meta(uint8_t opcode);

typedef void (*qe6502_log_fn)(void* context, const char* topic, const char* message);
QE_FFI_API(void)            qe6502_set_logger(qe6502_log_fn logger, void* context);

#if defined(QE6502_ENABLE_MEM_ALLOC) && (QE6502_ENABLE_MEM_ALLOC == 1)
    QE_FFI_API(void)        qe6502_cpu_pool_reset(void);
    QE_FFI_API(void*)       qe6502_cpu_alloc(void);
    QE_FFI_API(void)        qe6502_cpu_free(void* ptr);
#endif

// Use only if you are very familiar with theinternal implementation of the library.

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

// Use only if you are very familiar with the internal implementation of the library.
QE_FFI_API(void)    qe6502_offsets(qe6502_offsets_t* offsets);
QE_FFI_API(void)    qe6502_unsafe_overwrite(qe6502_cpu_t* cpu, uint64_t state);

#endif // QE6502_H__
