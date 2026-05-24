#ifndef QE6502_H
#define QE6502_H

#include <stdint.h>

#ifndef QE6502_STATIC_ASSERT
# ifdef __cplusplus
#  define QE6502_STATIC_ASSERT(condition, message) static_assert((condition), message)
# else
#  define QE6502_STATIC_ASSERT(condition, message) _Static_assert((condition), message)
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Model identifiers. */
enum
{
    qe6502_model_nmos = 0,
    qe6502_model_nes  = 1,
    qe6502_model_wdc  = 2,
    qe6502_model_rw   = 3,
    qe6502_model_st   = 4,

    qe6502_supported_models_count = 5
};

/* Control-store layout.
 *
 * The control store is split into fixed-size model blocks. Each model block
 * contains 512 slots: opcode slots 0x000..0x0FF and private service slots
 * 0x100..0x1FF. Each slot contains 8 microcode entries.
 *
 * Addressing form:
 *
 *     model block + slot + cycle -> microcode entry
 */
enum
{
    qe6502_cycle_bits          = 3,
    qe6502_slot_bits           = 9,
    qe6502_cycles_per_slot     = 1u << qe6502_cycle_bits,
    qe6502_slots_per_model     = 1u << qe6502_slot_bits,
    qe6502_entries_per_model   = qe6502_slots_per_model * qe6502_cycles_per_slot,
    qe6502_control_store_size  = qe6502_entries_per_model * qe6502_supported_models_count
};

/* CPU state. */
typedef struct qe6502_cpu
{
    /* Configuration. */
    uint8_t model;

    /* Internal execution state. */
    uint8_t  status;
    uint16_t microcode;
    uint16_t latch_addr;
    uint8_t latch_data;

    /* Reserved for future extensions. */
    uint8_t reserved_for_extension;

    /* CPU registers. */
    uint16_t PC;
    uint8_t  S;
    uint8_t  A;
    uint8_t  X;
    uint8_t  Y;
    uint8_t  P;

    /* Interrupts / control. */
    uint8_t interrupts;
} qe6502_t;
QE6502_STATIC_ASSERT(sizeof(qe6502_t) == 16, "qe6502_t must be 16 bytes");

/* Single bus-cycle result. */
typedef struct qe6502_tick_result
{
    uint16_t address;
    uint8_t bus;
    uint8_t status;
} qe6502_tick_t;

/* Processor status register flags. */
static const uint8_t qe6502_flag_C  = (1u << 0);
static const uint8_t qe6502_flag_Z  = (1u << 1);
static const uint8_t qe6502_flag_I  = (1u << 2);
static const uint8_t qe6502_flag_D  = (1u << 3);
static const uint8_t qe6502_flag_B  = (1u << 4);
static const uint8_t qe6502_flag_UN = (1u << 5);
static const uint8_t qe6502_flag_V  = (1u << 6);
static const uint8_t qe6502_flag_N  = (1u << 7);

/* Tick status flags. */
static const uint8_t qe6502_status_writing     = (1u << 0);
static const uint8_t qe6502_status_instr_done  = (1u << 1);
static const uint8_t qe6502_status_nmi_starts  = (1u << 2);
static const uint8_t qe6502_status_irq_starts  = (1u << 3);
static const uint8_t qe6502_status_halted      = (1u << 7);

/* Error status flags. */
static const uint8_t qe6502_error_illegal_op   = (1);

/* Microcode entry. */
typedef qe6502_tick_t (*qe6502_microcode_fn)(qe6502_t *cpu, uint8_t bus);

/* Control store. */
extern const qe6502_microcode_fn qe6502_control_store[qe6502_control_store_size];

/* Public API. */
qe6502_tick_t qe6502_v2_light_reset(qe6502_t* cpu);
qe6502_tick_t qe6502_v2_goto(qe6502_t* cpu, uint16_t address);

static inline qe6502_tick_t qe6502_tick(qe6502_t* cpu, uint8_t bus)
{
    qe6502_tick_t tick = qe6502_control_store[cpu->microcode](cpu, bus);
    cpu->microcode++;
    return tick;
}

#ifdef __cplusplus
}
#endif

#endif // QE6502_H
