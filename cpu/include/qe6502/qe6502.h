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

#ifndef QE6502_H
#define QE6502_H

#include <qe/abi_utils.h>

#include <stdint.h>
#include <stddef.h>

/** Logger callback used by qe6502_set_logger(). */
typedef void (QE_CALL *qe6502_log_fn)(void* context, const char* topic, const char* message);

#define QE6502_MODEL_MOS (0)
#define QE6502_MODEL_NES (1)

#define QE6502_MODEL_WDC (2)
#define QE6502_MODEL_RW  (3)
#define QE6502_MODEL_ST  (4)

#define QE6502_CONTEXT_SIZE   40u
#define QE6502_CONTEXT_ALIGN  8u

/** Opaque 6502 CPU context. */
typedef struct qe6502_cpu_t
{
    QE_ALIGNAS(QE6502_CONTEXT_ALIGN)
    unsigned char bytes[QE6502_CONTEXT_SIZE];
} qe6502_cpu_t;

/** 6502 addressing mode identifier used by qe6502_opcode_meta_t. */
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

/** Static metadata for a single opcode. */
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
    uint8_t is_illegal;
    uint8_t reserved_data[3];
} qe6502_opcode_meta_t;

/**
 * Packed result of a CPU tick.
 *
 * A tick describes the CPU bus request for the current cycle:
 * the memory address, whether the CPU is reading or writing,
 * optional data-out for write cycles, and a few status flags.
 *
 * Typical loop:
 *   - inspect the current tick;
 *   - if the CPU is reading, pass memory[QE6502_ADDRESS(tick)] to the next qe6502_tick();
 *   - if the CPU is writing, store QE6502_DATA(tick) at memory[QE6502_ADDRESS(tick)];
 *   - use the tick returned by qe6502_tick() as the request for the next cycle.
 */
typedef uint32_t qe6502_tick_t;
    // qe6502_tick_t bit offsets
    #define QE6502_TICK_ADDRESS_OFFS            (0)     // bits [ 0..15] : Memory address
    #define QE6502_TICK_DATA_OFFS               (16)    // bits [16..23] : Data out, valid only during CPU write cycles
    #define QE6502_TICK_WRITING_OFFS            (24)    // bits [24    ] : 1 == CPU write cycle, 0 == CPU read cycle
    #define QE6502_TICK_CPU_STARTING_OFFS       (25)    // bits [25    ] : 1 == Starting in progress, 0 == Started
    #define QE6502_TICK_INSTR_DONE_OFFS         (26)    // bit  [26    ] : 1 == Instruction boundary, 0 == Mid-instruction
                                                        // bits [27..30] : Reserved
    #define QE6502_TICK_NOT_OK_OFFS             (31)    // bits [31    ] : 1 == CPU error, 0 == CPU is OK
    // qe6502_tick_t getters
    #define QE6502_ADDRESS(tick)                ((uint16_t)(((tick) >> QE6502_TICK_ADDRESS_OFFS)    & 0xffff))
    #define QE6502_DATA(tick)                   ((uint8_t)(((tick) >> QE6502_TICK_DATA_OFFS)       & 0xff))
    #define QE6502_IS_WRITING(tick)             ((uint8_t)(((tick) >> QE6502_TICK_WRITING_OFFS)     & 1))
    #define QE6502_IS_STARTING(tick)            ((uint8_t)(((tick) >> QE6502_TICK_CPU_STARTING_OFFS)& 1))

    // Returns 1 at an instruction boundary.
    // State dumped at this point can be recovered;
    // mid-instruction dumps are inspect-only.
    #define QE6502_IS_INSTR_DONE(tick)          ((uint8_t)(((tick) >> QE6502_TICK_INSTR_DONE_OFFS)  & 1))

    #define QE6502_IS_NOT_OK(tick)              ((uint8_t)(((tick) >> QE6502_TICK_NOT_OK_OFFS)      & 1))
    #define QE6502_IS_OK(tick)                  ((uint8_t)(!QE6502_IS_NOT_OK(tick)))

/** Packed CPU state returned by qe6502_dump_state(). */
typedef uint64_t qe6502_state_t;
    // qe6502_state_t bit offsets                                              bits
    #define QE6502_STATE_PC_OFFS                (0)     // [ 0..15] : PC
    #define QE6502_STATE_A_OFFS                 (16)    // [16..23] : A
    #define QE6502_STATE_X_OFFS                 (24)    // [24..31] : X
    #define QE6502_STATE_Y_OFFS                 (32)    // [32..39] : Y
    #define QE6502_STATE_S_OFFS                 (40)    // [40..47] : S
    #define QE6502_STATE_P_OFFS                 (48)    // [48..55] : P
    #define QE6502_STATE_MODEL_OFFS             (56)    // [56..58] : CPU Model
                                                        // [59    ] : 0, Used internally
    #define QE6502_STATE_NMI_OFFS               (60)    // [60    ] : NMI pin
    #define QE6502_STATE_IRQ_OFFS               (61)    // [61    ] : IRQ pin
    #define QE6502_STATE_WAI_OFFS               (62)    // [62    ] : WAI state
    #define QE6502_STATE_NOT_RECOVERABLE_OFFS   (63)    // [63    ] : State is not recoverable
    // qe6502_state_t getters
    #define QE6502_PC(state)                    ((uint16_t)(((state) >> QE6502_STATE_PC_OFFS)              & 0xffff))
    #define QE6502_A(state)                     ((uint8_t)(((state) >> QE6502_STATE_A_OFFS)               & 0xff))
    #define QE6502_X(state)                     ((uint8_t)(((state) >> QE6502_STATE_X_OFFS)               & 0xff))
    #define QE6502_Y(state)                     ((uint8_t)(((state) >> QE6502_STATE_Y_OFFS)               & 0xff))
    #define QE6502_S(state)                     ((uint8_t)(((state) >> QE6502_STATE_S_OFFS)               & 0xff))
    #define QE6502_P(state)                     ((uint8_t)(((state) >> QE6502_STATE_P_OFFS)               & 0xff))
    #define QE6502_MODEL(state)                 ((uint8_t)(((state) >> QE6502_STATE_MODEL_OFFS)           & 0x7))
    #define QE6502_IS_NMI_HI(state)             ((uint8_t)(((state) >> QE6502_STATE_NMI_OFFS)             & 0x1))
    #define QE6502_IS_IRQ_HI(state)             ((uint8_t)(((state) >> QE6502_STATE_IRQ_OFFS)             & 0x1))
    #define QE6502_IS_WAI(state)                ((uint8_t)(((state) >> QE6502_STATE_WAI_OFFS)             & 0x1))
    #define QE6502_IS_NOT_RECOVERABLE(state)    ((uint8_t)(((state) >> QE6502_STATE_NOT_RECOVERABLE_OFFS) & 0x1))
    // qe6502_state_t setters
    #define QE6502_SET_PC(state, pc)            QE6502_SET_STATE_FIELD_HELPER((state), (pc), QE6502_STATE_PC_OFFS, 0xffff)
    #define QE6502_SET_A(state, a)              QE6502_SET_STATE_FIELD_HELPER((state), (a), QE6502_STATE_A_OFFS, 0xff)
    #define QE6502_SET_X(state, x)              QE6502_SET_STATE_FIELD_HELPER((state), (x), QE6502_STATE_X_OFFS, 0xff)
    #define QE6502_SET_Y(state, y)              QE6502_SET_STATE_FIELD_HELPER((state), (y), QE6502_STATE_Y_OFFS, 0xff)
    #define QE6502_SET_S(state, s)              QE6502_SET_STATE_FIELD_HELPER((state), (s), QE6502_STATE_S_OFFS, 0xff)
    #define QE6502_SET_P(state, p)              QE6502_SET_STATE_FIELD_HELPER((state), (p), QE6502_STATE_P_OFFS, 0xff)
    #define QE6502_SET_MODEL(state, model)      QE6502_SET_STATE_FIELD_HELPER((state), (model), QE6502_STATE_MODEL_OFFS, 0x7)
    #define QE6502_SET_NMI_HI(state, hi)        QE6502_SET_STATE_FIELD_HELPER((state), (hi), QE6502_STATE_NMI_OFFS, 0x1)
    #define QE6502_SET_IRQ_HI(state, hi)        QE6502_SET_STATE_FIELD_HELPER((state), (hi), QE6502_STATE_IRQ_OFFS, 0x1)
    #define QE6502_SET_WAI(state, wai)          QE6502_SET_STATE_FIELD_HELPER((state), (wai), QE6502_STATE_WAI_OFFS, 0x1)
    #define QE6502_SET_STATE_FIELD_HELPER(state, value, offs, mask) do {(state) = (qe6502_state_t)((((qe6502_state_t)(state)) & ~(((qe6502_state_t)(mask)) << (offs))) | ((((qe6502_state_t)(value)) & ((qe6502_state_t)(mask))) << (offs)));} while (0)

/** Returns the packed library version. */
QE_FFI_API(uint32_t)        qe6502_version(void);

/** Returns sizeof(qe6502_cpu_t). Useful for FFI bindings. */
QE_FFI_API(size_t)          qe6502_size(void);

/** Returns the required alignment of qe6502_cpu_t. Useful for FFI bindings. */
QE_FFI_API(size_t)          qe6502_align(void);

/** Starts the CPU reset sequence using the selected model. Returns the initial tick. */
QE_FFI_API(qe6502_tick_t)   qe6502_reset(qe6502_cpu_t* cpu, uint8_t model);

/** Advances the CPU by one tick using data for read cycles. Returns the resulting tick. */
QE_FFI_API(qe6502_tick_t)   qe6502_tick(qe6502_cpu_t* cpu, uint8_t data);

/**
 * Sets NMI level without ticking the CPU.
 * Note: 1 drives the active-low NMI pin low a high-to-low transition latches NMI.
 *       0 drives the pin high.
 */
QE_FFI_API(qe6502_tick_t)   qe6502_set_nmi(qe6502_cpu_t* cpu, uint8_t low);

/**
 * Sets IRQ level. Note: this API uses asserted semantics; pass 1 to drive the
 * active-low IRQ pin low and assert the interrupt, or 0 to deassert it.
 */
QE_FFI_API(qe6502_tick_t)   qe6502_set_irq(qe6502_cpu_t* cpu, uint8_t low);

/** Restores a recoverable CPU state. Returns the resulting tick. */
QE_FFI_API(qe6502_tick_t)   qe6502_recover(qe6502_cpu_t* cpu, qe6502_state_t stable_state);

/** Returns the last tick without changing CPU state. */
QE_FFI_API(qe6502_tick_t)   qe6502_last_tick(const qe6502_cpu_t* cpu);

/**
 * Dumps the current CPU state. Mid-instruction dumps are inspect-only.
 * A state is recoverable only when qe6502_dump_state() is called immediately
 * after a tick for which QE6502_IS_INSTR_DONE(tick) is true.
 */
QE_FFI_API(qe6502_state_t)  qe6502_dump_state(const qe6502_cpu_t* cpu);

/** Returns the current CPU error code, or 0 if the CPU is OK. */
QE_FFI_API(uint16_t)        qe6502_error_code(const qe6502_cpu_t* cpu);

/** Returns a human-readable description of the current CPU error. */
QE_FFI_API(const char*)     qe6502_error_string(const qe6502_cpu_t* cpu);

/** Decodes an error code returned by qe6502_error_code(). */
QE_FFI_API(const char*)     qe6502_decode_error(uint16_t error_code);

/** Returns static metadata for opcode. The returned pointer is owned by the library. */
QE_FFI_API(const qe6502_opcode_meta_t*) qe6502_opcode_meta(uint8_t opcode, uint8_t cpu_model);

/** Sets the global logger callback. */
QE_FFI_API(void)            qe6502_set_logger(qe6502_log_fn logger, void* context);

#endif // QE6502_H
