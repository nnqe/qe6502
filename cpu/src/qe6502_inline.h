#ifndef QE6502_INLINE_H
#define QE6502_INLINE_H

#include <qe/log.h>
#include <qe/impl_utils.h>
#include "qe6502_defs.h"
#include <stddef.h>

#define INSTR_FW_DECL(funcname) INSTR_RETTYPE qe6502_cycle_t funcname( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
#define OFFSETOF(member) offsetof( qe6502_t, member )
#define INSTR_RETTYPE QE_SIC
#define INSTR_ARGS

#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    QE_PRIVATE_API(qe6502_cycle_t) mos_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_PRIVATE_API(qe6502_cycle_t) nes_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    QE_PRIVATE_API(qe6502_cycle_t) rw_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_PRIVATE_API(qe6502_cycle_t) wdc_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_PRIVATE_API(qe6502_cycle_t) st_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif

static const uint32_t writing_packed_cmd = (1 << 24); //(uint32_t)qe6502_writing << 24;

QE_SIC
qe6502_cycle_t resume_to(qe6502_microcode_fn handler)
{
    return (qe6502_cycle_t){ .execute = handler };
}

QE_MAYBE_UNUSED
QE_SIC
qe6502_cycle_t resume_to_dummy_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
#   if(QE6502_ENABLE_CYCLE_MERGE == 1)
        cpu->merged++;
        return handler(cpu);
#   else
        (void)cpu;
        return (qe6502_cycle_t){ .execute = handler };
#   endif
}

QE_MAYBE_UNUSED
QE_SIC
qe6502_cycle_t resume_to_dummy_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
#   if(QE6502_ENABLE_CYCLE_MERGE == 1)
        cpu->merged++;
        return handler(cpu);
#   else
        (void)cpu;
        return (qe6502_cycle_t){ .execute = handler };
#   endif
}

QE_SIC
qe6502_cycle_t jump_to( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
    return handler(cpu);
}

QE_MAYBE_UNUSED
QE_SIC
void request_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word16_t read_address, uint8_t store_offs)
{
    cpu->cmd.packed =   QE_U32(read_address.u16) |
                        QE_U32(QE_U32(store_offs) << 16);
}

QE_MAYBE_UNUSED
QE_SIC
void request_stack_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t store_offs)
{
    cpu->cmd.packed =   QE_U32(1 << 8) |
                        QE_U32(stack_address) |
                        QE_U32(QE_U32(store_offs) << 16);
}

QE_MAYBE_UNUSED
QE_SIC
void request_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word16_t write_address, uint8_t get_offset)
{
    cpu->cmd.packed =   QE_U32(write_address.u16) |
                        QE_U32(QE_U32(get_offset) << 16) |
                        QE_U32(writing_packed_cmd);
}

QE_MAYBE_UNUSED
QE_SIC
void request_stack_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t get_offset)
{
    cpu->cmd.packed =   QE_U32(1 << 8) |
                        QE_U32(stack_address) |
                        QE_U32(QE_U32(get_offset << 16)) |
                        QE_U32(writing_packed_cmd);
}

QE_MAYBE_UNUSED
QE_SIC
void request_read_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word16_t read_address, uint8_t store_offs)
{
#   if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =   QE_U32(read_address.u16) |
                            QE_U32(QE_U32(store_offs) << 16);
#   else
        (void)cpu; (void)read_address; (void)store_offs;
#   endif
}

QE_MAYBE_UNUSED
QE_SIC
void request_stack_read_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t store_offs)
{
#   if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =   QE_U32(1 << 8) |
                            QE_U32(stack_address) |
                            QE_U32(QE_U32(store_offs) << 16);
#   else
        (void)cpu; (void)stack_address; (void)store_offs;
#   endif
}

QE_MAYBE_UNUSED QE_SIC
void request_write_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word16_t write_address, uint8_t get_offset)
{
#   if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =   QE_U32(write_address.u16) |
                            QE_U32(get_offset) << 16 |
                            QE_U32(writing_packed_cmd);
#   else
        (void)cpu; (void)write_address; (void)get_offset;
#   endif
}

QE_MAYBE_UNUSED
QE_SIC
void request_stack_write_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t get_offset)
{
#   if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =   QE_U32(1 << 8) |
                            QE_U32(stack_address) |
                            QE_U32(get_offset) << 16 |
                            QE_U32(writing_packed_cmd);
#   else
        (void)cpu; (void)stack_address; (void)get_offset;
#   endif
}

QE_MAYBE_UNUSED
QE_SIC
void update_flags( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t mask, uint8_t values)
{
    cpu->P = QE_U8(( cpu->P & QE_U8(~mask) ) | ( values ));
}

INSTR_RETTYPE qe6502_cycle_t
halt( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->istate |= qe6502_halted;
    cpu->cmd.flags |= qe6502_halted;
    return resume_to(halt);
}

QE_MAYBE_UNUSED
QE_SIC
qe6502_cycle_t cpu_error( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t error_code)
{
    qe_log_error("CPU Error: %u (%02X)", (unsigned)error_code, (unsigned)error_code);
    cpu->error_code = ( error_code );
    return jump_to(cpu, halt);
}

#endif // QE6502_INLINE_H

