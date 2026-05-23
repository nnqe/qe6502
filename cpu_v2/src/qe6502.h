#ifndef QE6502_H
#define QE6502_H

#include <stdarg.h>
#include <qe/impl_utils.h>

enum
{
    qe6502_model_nmos = 0,
    qe6502_model_nes  = 1,
    qe6502_model_wdc  = 2,
    qe6502_model_rw   = 3,
    qe6502_model_st   = 4,

    qe6502_supported_models_count = 5,

    qe6502_microcode_cycle_bits        = 3,
    qe6502_microcode_flow_bits         = 9,
    qe6502_microcode_cycles_per_flow   = 1u << qe6502_microcode_cycle_bits,
    qe6502_microcode_flows_per_model   = 1u << qe6502_microcode_flow_bits,
    qe6502_microcode_entries_per_model = qe6502_microcode_flows_per_model * qe6502_microcode_cycles_per_flow,
    qe6502_microcode_table_size        = qe6502_microcode_entries_per_model * qe6502_supported_models_count
};

typedef struct qe6502
{
    /* Configuration */
    uint8_t model;

    /* Internal execution state */
    uint8_t  status;
    uint16_t microcode;
    uint16_t latch_addr;
    uint8_t latch_data;

    /* Reserved for future extensions */
    uint8_t reserved_for_extension;

    /* CPU registers */
    uint16_t PC;
    uint8_t  S;
    uint8_t  A;
    uint8_t  X;
    uint8_t  Y;
    uint8_t  P;

    /* Interrupts / control */
    uint8_t interrupts;
} qe6502_t;
QE_STATIC_ASSERT(sizeof(qe6502_t) == 16, "qe6502_t must be 16 bytes");

typedef struct qe6502_tick
{
    uint16_t address;
    uint8_t bus;
    uint8_t status;
} qe6502_tick_t;

static const uint8_t qe6502_flag_C  = ( 1 << 0 ); //qe6502_flagpos_C
static const uint8_t qe6502_flag_Z  = ( 1 << 1 ); //qe6502_flagpos_Z
static const uint8_t qe6502_flag_I  = ( 1 << 2 ); //qe6502_flagpos_I
static const uint8_t qe6502_flag_D  = ( 1 << 3 ); //qe6502_flagpos_D
static const uint8_t qe6502_flag_B  = ( 1 << 4 ); //qe6502_flagpos_B
static const uint8_t qe6502_flag_UN = ( 1 << 5 ); //qe6502_flagpos_UN
static const uint8_t qe6502_flag_V  = ( 1 << 6 ); //qe6502_flagpos_V
static const uint8_t qe6502_flag_N  = ( 1 << 7 ); //qe6502_flagpos_N

QE_MAYBE_UNUSED static const uint8_t qe6502_status_writing     = (1 << 0);
QE_MAYBE_UNUSED static const uint8_t qe6502_status_instr_done  = (1 << 1);
QE_MAYBE_UNUSED static const uint8_t qe6502_status_nmi_starts  = (1 << 2);
QE_MAYBE_UNUSED static const uint8_t qe6502_status_irq_starts  = (1 << 3);
QE_MAYBE_UNUSED static const uint8_t qe6502_status_halted      = (1 << 7);

QE_MAYBE_UNUSED static const uint8_t qe6502_error_illegal_op   = (1);

typedef qe6502_tick_t (*qe6502_microcode_fn)(qe6502_t *cpu, uint8_t bus);

extern const qe6502_microcode_fn qe6502_microcode_table[qe6502_microcode_table_size];

static inline qe6502_tick_t qe6502_tick(qe6502_t* cpu, uint8_t bus)
{
    qe6502_tick_t tick = qe6502_microcode_table[cpu->microcode](cpu, bus);
    cpu->microcode++;
    return tick;
}

#endif // QE6502_H
