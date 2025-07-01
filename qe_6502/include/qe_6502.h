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

#ifndef QE_QE6502_H__
#define QE_QE6502_H__

#include <qe_utils.h>

// Version
#define QE6502_VERSION_MAJOR 1
#define QE6502_VERSION_MINOR 3

#define QE6502_VERSION  (uint16_t)(((uint16_t)QE6502_VERSION_MAJOR << 8) | ((uint16_t)QE6502_VERSION_MINOR   << 0))

// Major++ on ABI break, Minor++ on backwards-compatible extension.
// Any pointer may be NULL.
QE_API
void qe6502_version(uint16_t* version,
                    uint8_t* version_major,
                    uint8_t* version_minor);

//
// Strongly recommended to keep QE6502_ENABLE_CYCLE_MERGE **OFF**
// unless you have a very specific benchmark-only use-case and
// fully accept the loss of true cycle accuracy.
// (~7–8 % speed-up, **below 10 %**)
//
// NOTE ON TEST COVERAGE vs QE6502_ENABLE_CYCLE_MERGE
//
// • With QE6502_ENABLE_CYCLE_MERGE == 0  (default, full cycle-exact):
//      – Passes the complete Klaus2m5 functional suite, including
//        decimal-mode edge cases;
//      – Passes *all* single-instruction timing tests that verify every
//        intermediate bus read/write performed inside each opcode.
//
// • With QE6502_ENABLE_CYCLE_MERGE == 1  (fast, dummy cycles removed):
//      – Still passes Klaus2m5 functional tests;
//      – **Fails** single-instruction bus-timing tests, because the merged
//        execution no longer presents the individual internal bus cycles
//        those tests expect to observe.
// ------------------------------------------------------------------------


typedef struct qe6502_cycle_t qe6502_cycle_t;
struct qe6502;
typedef qe6502_cycle_t (*qe6502_microcode_fn)( struct qe6502* QE_RESTRICT cpu );

struct qe6502_cycle_t
{
    qe6502_microcode_fn execute;
};

#ifdef QE_LITTLE_ENDIAN
typedef union
{
    struct
    {
        uint16_t address;
        uint8_t offset;
        uint8_t flags;
    };
    uint32_t packed;
} qe6502_cmd_t;
#else
typedef union
{
    struct
    {
        uint8_t flags;
        uint8_t offset;
        uint16_t address;
    };
    uint32_t packed;
} qe6502_cmd_t;
#endif
QE_STATIC_ASSERT(sizeof(qe6502_cmd_t) == 4, "qe6502_cmd_t must be 4 bytes");

typedef struct
{
    qe6502_microcode_fn entry;
    qe6502_microcode_fn instr;
} qe6502_opcode_info_t;

typedef struct
{
    const qe6502_opcode_info_t opcodes[256];
} qe6502_model_t;

typedef struct qe6502
{
    /******** user interface ********/
    qe6502_cmd_t cmd;                   // 4

    /******** microcode state ********/
    qe_word_t address;                  // 2
    union
    {
        qe_word_t pointer;              // 2
        uint16_t error_code;
    };
    qe6502_microcode_fn instr;          // 8|4|2
    uint8_t merged;
    union
    {
        uint8_t data;                   // 1
        int8_t pagecross_overflow;
    };
    uint8_t model;
    uint8_t opcode;                     // 1

    /******** cpu data ********/
    qe_word_t PC;                       // 2
    uint8_t S;                          // 5 x 1
    uint8_t A;
    uint8_t X;
    uint8_t Y;
    uint8_t P;
} qe6502_t;
QE_STATIC_ASSERT(sizeof(qe6502_t) <= 32, "qe6502_t must be at most 32 bytes");

// Set during the *final* cycle of an instruction,
// i.e. the cycle that **requests** the opcode fetch
// for the next instruction. The host must handle this
// flag *before* it performs the bus transfer, otherwise
// cycle-exact traces shift by one tick.
static const uint8_t qe6502_writing     = (1 << 0);
static const uint8_t qe6502_starting    = (1 << 1);
static const uint8_t qe6502_wait_opcode = (1 << 2);
static const uint8_t qe6502_halted      = (1 << 3);

// Errors
static const uint8_t qe6502_err_compile_error   = (1 << 0);
static const uint8_t qe6502_err_illegal_instr   = (1 << 1);
static const uint8_t qe6502_err_poweron_error   = (1 << 2);
static const uint8_t qe6502_err_logic_error     = (1 << 3);
static const uint8_t qe6502_err_unknown_model  = (1 << 4);

#if (QE6502_ENABLE_NMOS_6502 == 1)
    const static uint8_t qe6502_mos = 0;
    const static uint8_t qe6502_nes = 1;
#endif
#if (QE6502_ENABLE_CMOS_65C02 == 1)
    const static uint8_t qe6502_wdc = 2;
    const static uint8_t qe6502_rw = 3;
    const static uint8_t qe6502_st = 4;
#endif

static const uint8_t qe6502_flagpos_C = 0;
static const uint8_t qe6502_flagpos_Z = 1;
static const uint8_t qe6502_flagpos_I = 2;
static const uint8_t qe6502_flagpos_D = 3;
static const uint8_t qe6502_flagpos_B = 4;
static const uint8_t qe6502_flagpos_UN = 5;
static const uint8_t qe6502_flagpos_V = 6;
static const uint8_t qe6502_flagpos_N = 7;

static const uint8_t qe6502_flag_C  = ( 1 << 0 ); //qe6502_flagpos_C
static const uint8_t qe6502_flag_Z  = ( 1 << 1 ); //qe6502_flagpos_Z
static const uint8_t qe6502_flag_I  = ( 1 << 2 ); //qe6502_flagpos_I
static const uint8_t qe6502_flag_D  = ( 1 << 3 ); //qe6502_flagpos_D
static const uint8_t qe6502_flag_B  = ( 1 << 4 ); //qe6502_flagpos_B
static const uint8_t qe6502_flag_UN = ( 1 << 5 ); //qe6502_flagpos_UN
static const uint8_t qe6502_flag_V  = ( 1 << 6 ); //qe6502_flagpos_V
static const uint8_t qe6502_flag_N  = ( 1 << 7 ); //qe6502_flagpos_N

QE_API qe6502_cycle_t
qe6502_power_on(qe6502_t* cpu, uint8_t model);

QE_API qe6502_cycle_t
qe6502_reset_instruction(qe6502_t* cpu); // Debug/test-only utility; do not use in normal operation.

QE_SIC qe_bool  qe6502_ok(const qe6502_t* cpu) { return !(cpu->cmd.flags & qe6502_halted); }
QE_SIC qe_bool  qe6502_needs_data(const qe6502_t* cpu) { return !(cpu->cmd.flags & qe6502_writing); }
QE_SIC qe_bool  qe6502_has_data(const qe6502_t* cpu) { return (cpu->cmd.flags & qe6502_writing)?1:0; }
QE_SIC void     qe6502_feed_data(qe6502_t* cpu, uint8_t byte) { ((uint8_t*)(cpu))[cpu->cmd.offset] = byte; }
QE_SIC uint8_t  qe6502_data(const qe6502_t* cpu) { return ((uint8_t*)(cpu))[cpu->cmd.offset]; }
QE_SIC uint16_t qe6502_address(const qe6502_t* cpu) { return cpu->cmd.address; }
QE_SIC qe_bool  qe6502_instr_done(const qe6502_t* cpu) { return (cpu->cmd.flags & qe6502_wait_opcode)?1:0; }
QE_SIC qe_bool  qe6502_started(const qe6502_t* cpu) { return qe6502_ok(cpu) && ((cpu->cmd.flags & qe6502_starting)?0:1); }

typedef struct
{
    uint8_t address;
    uint8_t pointer;
    uint8_t error_code;
    uint8_t data;
    uint8_t pagecross_overflow;
    uint8_t opcode;
    uint8_t PC;
    uint8_t S;
    uint8_t A;
    uint8_t X;
    uint8_t Y;
    uint8_t P;
} qe6502_offsets_t;

QE_API void
qe6502_offsets(qe6502_offsets_t* offsets);

#endif // QE_QE6502_H__
