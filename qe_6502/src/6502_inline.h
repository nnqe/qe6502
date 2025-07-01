#ifndef QE_6502_INLINE_H__
#define QE_6502_INLINE_H__

#include <qe_6502.h>
#include <stddef.h>

#define INSTR_FW_DECL(funcname) INSTR_RETTYPE qe6502_cycle_t funcname( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
#define OFFSETOF(member) offsetof( qe6502_t, member )
#define INSTR_RETTYPE QE_SIC
#define INSTR_ARGS

static const uint32_t writing_packed_cmd = (1 << 24); //(uint32_t)qe6502_writing << 24;

QE_SIC
qe6502_cycle_t resume_to(qe6502_microcode_fn handler)
{
    return (qe6502_cycle_t){ .execute = handler };
}

QE_SIC
qe6502_cycle_t resume_to_dummy_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
    #if(QE6502_ENABLE_CYCLE_MERGE == 1)
        cpu->merged++;
        return handler(cpu);
    #else
        (void)cpu;
        return (qe6502_cycle_t){ .execute = handler };
    #endif
}

QE_SIC
qe6502_cycle_t resume_to_dummy_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
    #if(QE6502_ENABLE_CYCLE_MERGE == 1)
        cpu->merged++;
        return handler(cpu);
    #else
        (void)cpu;
        return (qe6502_cycle_t){ .execute = handler };
    #endif
}

QE_SIC
qe6502_cycle_t jump_to( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe6502_microcode_fn handler)
{
    return handler(cpu);
}

QE_SIC void request_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word_t read_address, uint8_t store_offs)
{
    cpu->cmd.packed = read_address.u16 |
                      (store_offs << 16);
}

QE_SIC void request_stack_read( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t store_offs)
{
    cpu->cmd.packed = ((1 << 8) | stack_address) |
                      (store_offs << 16);
}

QE_SIC void request_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word_t write_address, uint8_t get_offset)
{
    cpu->cmd.packed =    write_address.u16 |
                        (get_offset << 16) |
                        writing_packed_cmd;
}

QE_SIC void request_stack_write( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t get_offset)
{
    cpu->cmd.packed =   ((1 << 8) | stack_address) |
                        (get_offset << 16) |
                        writing_packed_cmd;
}

QE_SIC void request_read_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word_t read_address, uint8_t store_offs)
{
    #if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed = read_address.u16 |
                          (store_offs << 16);
    #endif
}

QE_SIC void request_stack_read_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t store_offs)
{
    #if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed = ((1 << 8) | stack_address) |
                          (store_offs << 16);
    #endif
}

QE_SIC void request_write_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, qe_word_t write_address, uint8_t get_offset)
{
    #if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =    write_address.u16 |
                            (get_offset << 16) |
                            writing_packed_cmd;
    #endif
}

QE_SIC void request_stack_write_dummy( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t stack_address, uint8_t get_offset)
{
    #if(QE6502_ENABLE_CYCLE_MERGE != 1)
        cpu->cmd.packed =   ((1 << 8) | stack_address) |
                            (get_offset << 16) |
                            writing_packed_cmd;
    #endif
}

QE_SIC void update_flags( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t mask, uint8_t values)
{
    cpu->P = ( cpu->P & (~ mask) ) | ( values );
}

INSTR_RETTYPE qe6502_cycle_t
halt( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->cmd.flags |= qe6502_halted;
    return resume_to(halt);
}

QE_SIC
qe6502_cycle_t cpu_error( INSTR_ARGS qe6502_t* QE_RESTRICT cpu, uint8_t error_code)
{
    qe_log("qe6502", "CPU Error: %u (%02X)", (unsigned)error_code, (unsigned)error_code);
    cpu->error_code = ( error_code );
    return jump_to(cpu, halt);
}

#endif // QE_6502_INLINE_H__

