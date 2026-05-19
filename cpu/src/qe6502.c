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

#include <qe/log.h>
#include <qe/impl_utils.h>
#include <qe6502/qe6502.h>
#include "qe6502_inline.h"
#include "qe6502_defs.h"

#if (!defined(QE6502_ENABLE_NMOS_6502)) || (QE6502_ENABLE_NMOS_6502 != 1)
#if (!defined(QE6502_ENABLE_CMOS_65C02)) || (QE6502_ENABLE_CMOS_65C02 != 1)

#error "Invalid qe6502 build: at least one of QE6502_ENABLE_NMOS_6502 or QE6502_ENABLE_CMOS_65C02 must be enabled."

#endif
#endif

QE_INTERNAL_API(void)
qe6502_version_impl(uint16_t* version,
                    uint8_t* version_major,
                    uint8_t* version_minor)
{
    if (version)        *version = QE_U16(QE6502_VERSION);
    if (version_major)  *version_major = QE_U8(QE6502_VERSION_MAJOR);
    if (version_minor)  *version_minor = QE_U8(QE6502_VERSION_MINOR);
}

/********************************************************
 *
 *  Power on sequence
 *
 ********************************************************/

static qe6502_cycle_t select_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
#   if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
        if(qe6502_mos == qe6502_model_impl(cpu))   return mos_fetch_opcode_bridge(cpu);
        if(qe6502_nes == qe6502_model_impl(cpu))   return nes_fetch_opcode_bridge(cpu);
#   endif
#   if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
        if(qe6502_wdc == qe6502_model_impl(cpu))   return wdc_fetch_opcode_bridge(cpu);
        if(qe6502_rw == qe6502_model_impl(cpu))    return rw_fetch_opcode_bridge(cpu);
        if(qe6502_st == qe6502_model_impl(cpu))    return st_fetch_opcode_bridge(cpu);
#   endif
    qe_log_error("Unknown model: %d", (unsigned)(qe6502_model_impl(cpu)));
    return cpu_error(cpu,  qe6502_err_unknown_model );
}

INSTR_RETTYPE qe6502_cycle_t
power_on_routine( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->opcode++;
    cpu->P = (1 << 5) | (1 << 2);
    switch (cpu->opcode)
    {
    case 2:
        cpu->PC.u16++;
        request_read(cpu, cpu->PC, OFFSETOF(data));
        cpu->cmd.flags = qe6502_starting;
        return resume_to(power_on_routine);

    case 3:
    case 4:
    case 5:
        // Cycle 3,4,5
        // 6502 quirk: RESET issues three dummy stack “pushes”
        // with R/W held HIGH (read-only) to drop SP to $FD
        // no data is written; not a bug.
        request_stack_read(cpu, cpu->S, OFFSETOF(data));
        cpu->cmd.flags = qe6502_starting;
        cpu->S--;
        return resume_to(power_on_routine);

    case 6:
        cpu->address.u16 = 0xFFFC;
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_0));
        cpu->cmd.flags = qe6502_starting;
        return resume_to(power_on_routine);

    case 7:
        cpu->address.u16++;
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_1));
        cpu->cmd.flags = qe6502_starting;
        return resume_to(power_on_routine);

    case 8:
        qe_log_info("Boot OK");
        return select_fetch_opcode_bridge(cpu);

    default:
        break;
    }
    return cpu_error(cpu,  qe6502_err_boot_error );
}

INSTR_RETTYPE qe6502_cycle_t
reset_to_normal_state( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->cmd.flags = 0;
    return select_fetch_opcode_bridge(cpu);
}

QE_INTERNAL_API(qe6502_cycle_t)
qe6502_reset_impl(qe6502_t* cpu, uint8_t model)
{
    qe_log_info("Power ON");
    qe_log_info("CPU size: %u", (unsigned)sizeof(qe6502_t));
    qe_log_info("CPU Cycle size: %u", (unsigned)sizeof(qe6502_cycle_t));
    QE_CLEAR_OBJ(*cpu);

    // test read
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word16_t){.u16=0xDEAD}, 0xA2);
    if (cpu->cmd.address != 0xDEAD ||
        cpu->cmd.offset != 0xA2)
    {
        qe_log_error("request_read");
        return cpu_error(cpu, qe6502_err_compile_error );
    }

    // test stack read
    cpu->cmd.packed = 0;
    request_stack_read(cpu, 0xDE, 0xAD);
    if (cpu->cmd.address != 0x01DE ||
        cpu->cmd.offset != 0xAD)
    {
        qe_log_error("request_stack_read");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test write
    cpu->cmd.packed = 0;
    request_write(cpu, (qe_word16_t){.u16=0xDEAD}, 0xA2);
    if (cpu->cmd.address != 0xDEAD ||
        cpu->cmd.offset != 0xA2 ||
        cpu->cmd.flags != qe6502_writing)
    {
        qe_log_error("request_write");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test stack write
    cpu->cmd.packed = 0;
    request_stack_write(cpu, 0xDE, 0xAD);
    if (cpu->cmd.address != 0x01DE ||
        cpu->cmd.offset != 0xAD ||
        cpu->cmd.flags != qe6502_writing)
    {
        qe_log_error("request_stack_write");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test offsets
    ((uint8_t*)(cpu))[ OFFSETOF(address.u8_0) ] = 0xAD;
    ((uint8_t*)(cpu))[ OFFSETOF(address.u8_1) ] = 0xDE;
    ((uint8_t*)(cpu))[ OFFSETOF(pointer.u8_0) ] = 0xEF;
    ((uint8_t*)(cpu))[ OFFSETOF(pointer.u8_1) ] = 0xBE;
    ((uint8_t*)(cpu))[ OFFSETOF(PC.u8_0) ] = 0xFE;
    ((uint8_t*)(cpu))[ OFFSETOF(PC.u8_1) ] = 0xCA;
    ((uint8_t*)(cpu))[ OFFSETOF(opcode) ] = 0xAB;
    ((uint8_t*)(cpu))[ OFFSETOF(data) ] = 0xBA;
    ((uint8_t*)(cpu))[ OFFSETOF(S) ] = 0xD8;
    ((uint8_t*)(cpu))[ OFFSETOF(A) ] = 0xC1;
    ((uint8_t*)(cpu))[ OFFSETOF(X) ] = 0xB2;
    ((uint8_t*)(cpu))[ OFFSETOF(Y) ] = 0xF3;
    ((uint8_t*)(cpu))[ OFFSETOF(P) ] = 0xA4;

    if (cpu->address.u16 != 0xDEAD ||
        cpu->pointer.u16 != 0xBEEF ||
        cpu->PC.u16 != 0xCAFE ||
        cpu->opcode != 0xAB ||
        cpu->data != 0xBA ||
        cpu->S != 0xD8 ||
        cpu->A != 0xC1 ||
        cpu->X != 0xB2 ||
        cpu->Y != 0xF3 ||
        cpu->P != 0xA4 ||
        qe6502_flagpos_N != 7 ||
        qe6502_flag_N != 128 ||
        qe6502_flagpos_C != 0 ||
        qe6502_flag_C != 1)
    {
        qe_log_error("CPU internal offsets");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // endian check
    cpu->address.u8_1 = 0x79;
    cpu->address.u8_0 = 0x09;
    if (cpu->address.u16 != 0x7909)
    {
        qe_log_error("little/big endian check");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    cpu->cmd.packed = 0xC001BABE;
    if (cpu->cmd.address != 0xBABE ||
        cpu->cmd.offset != 0x01 ||
        cpu->cmd.flags != 0xC0)
    {
        qe_log_error("little/big endian 32 check");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // istate
    cpu->istate = 0;

    // check model
    if ( (qe6502_model_max + 1) & qe6502_model_max )
    {
        qe_log_error("qe6502_model_max should be mask");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    if (qe6502_model_max >= qe6502_nmi_pin_chg ||
        qe6502_model_max >= qe6502_nmi_pin_lo  ||
        qe6502_model_max >= qe6502_irq_pin_lo )
    {
        qe_log_error("istate pin values must be greater than qe6502_model_max");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    if (model > qe6502_model_max)
    {
        qe_log_error("Model id too big");
        return cpu_error(cpu,  qe6502_err_unknown_model );
    }
    cpu->model = model;
#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    if(qe6502_mos == qe6502_model_impl(cpu))   qe_log_info("Model set to 'MOS 6502'");       else
    if(qe6502_nes == qe6502_model_impl(cpu))   qe_log_info("Model set to 'NES 6502'");       else
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    if(qe6502_wdc == qe6502_model_impl(cpu))   qe_log_info("Model set to 'WDC 65C02'");      else
    if(qe6502_rw == qe6502_model_impl(cpu))    qe_log_info("Model set to 'Rockwell 65C02'"); else
    if(qe6502_st == qe6502_model_impl(cpu))    qe_log_info("Model set to 'Synertek 65C02'"); else
#endif
    {
        qe_log_error("unknown model: %d", (unsigned)model);
        return cpu_error(cpu,  qe6502_err_unknown_model );
    }

    // test API
    cpu->istate = qe6502_halted;
    if (qe6502_ok_impl(cpu))
    {
        qe_log_error("'qe6502_ok' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->istate = 0;
    if (!qe6502_ok_impl(cpu))
    {
        qe_log_error("'qe6502_ok' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word16_t){.u16=0xDEAD}, 0xA2);
    if (!qe6502_needs_data_impl(cpu))
    {
        qe_log_error("'qe6502_needs_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_has_data_impl(cpu))
    {
        qe_log_error("'qe6502_has_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_write(cpu, (qe_word16_t){.u16=0xDEAD}, 0xA2);
    if (qe6502_needs_data_impl(cpu))
    {
        qe_log_error("'qe6502_needs_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (!qe6502_has_data_impl(cpu))
    {
        qe_log_error("'qe6502_has_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_address_impl(cpu) != 0xDEAD)
    {
        qe_log_error("'qe6502_address' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = qe6502_wait_opcode;
    if (!qe6502_instr_done_impl(cpu))
    {
        qe_log_error("'qe6502_instr_done' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = 0;
    if (qe6502_instr_done_impl(cpu))
    {
        qe_log_error("'qe6502_instr_done' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word16_t){.u16=0xBEEF}, OFFSETOF(data));
    qe6502_feed_data_impl(cpu, 0x42);
    if (cpu->data != 0x42)
    {
        qe_log_error("'qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    request_write(cpu, (qe_word16_t){.u16=0xBEEF}, OFFSETOF(data));
    if (qe6502_data_impl(cpu) != 0x42)
    {
        qe_log_error("'qe6502_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_cycle_t cycle = qe6502_reset_instruction_impl(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok_impl(cpu) ||
        !qe6502_instr_done_impl(cpu))
    {
        qe_log_error("'qe6502_reset_instruction' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_feed_data_impl(cpu, 0xEA);
    if (cpu->opcode != 0xEA)
    {
        qe_log_error("'qe6502_reset_instruction or qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cycle = cycle.execute(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok_impl(cpu))
    {
        qe_log_error("'internal cycle exec' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    qe_log_info("Self-test OK");

#if(QE6502_ENABLE_CYCLE_MERGE == 1)
    qe_log_warn("Cycles merge enabled!");
#endif

    // We hope everything's good
    // Let's go ...

    QE_CLEAR_OBJ(*cpu);
    cpu->model = model; // this will also clear internal istate

    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->cmd.flags = qe6502_starting;
    cpu->opcode = 1;
    return resume_to(power_on_routine);
}

QE_INTERNAL_API(qe6502_cycle_t)
qe6502_reset_instruction_impl(qe6502_t *cpu)
{
    cpu->P |= qe6502_flag_UN;

    return select_fetch_opcode_bridge(cpu);
}

/*
 * Returns a packed CPU state and BUS operation *
 * Bit layout:
 *
 *   bits [ 0..15] : Memory address
 *   bits [16..23] : Data out, valid only when bit [16] == 1
 *   bits [24    ] : Bus direction, 0 == Read request, 1 == Write request
 *   bits [25    ] : 0 == Started | 1 == Starting
 *   bits [26    ] : 0 == During instruction | 1 == Instruction done
 *   bits [27..30] : Reserved
 *   bits [31    ] : 0 == OK | 1 == Halted / not OK
 *
 * This is a numeric bit encoding. Decode with shifts and masks.
 */
QE_SIC qe6502_tick_t qe6502_get_tick_impl(const qe6502_t* cpu)
{
    qe_word32_t packed;
    packed.word16_0.u16 = cpu->cmd.address;
    packed.u8_2 = qe6502_has_data_impl(cpu) ? qe6502_data_impl(cpu) : 0;
    packed.u8_3 = cpu->cmd.flags;
    return packed.u32;
}

QE_SIC void qe6502_overwrite_impl(qe6502_t* cpu, qe6502_state_t state)
{
    cpu->PC     = qe_as_word64(state).word16_0;
    cpu->A      = qe_as_word64(state).u8_2;
    cpu->X      = qe_as_word64(state).u8_3;
    cpu->Y      = qe_as_word64(state).u8_4;
    cpu->S      = qe_as_word64(state).u8_5;
    cpu->P      = qe_as_word64(state).u8_6;
    cpu->model  = qe_as_word64(state).u8_7; // model and istate shared the same byte
}

// Foreign Function Interface (FFI)

typedef struct qe6502_cpuwrap
{
    qe6502_t cpu;
    qe6502_cycle_t cycle;
} qe6502_cpuwrap_t;

QE_STATIC_ASSERT(sizeof(qe6502_cpuwrap_t) <= QE6502_CONTEXT_SIZE,
                 "qe6502_cpu_t is too small");

QE_STATIC_ASSERT(_Alignof(qe6502_cpuwrap_t) <= QE6502_CONTEXT_ALIGN,
                 "qe6502_cpu_t alignment is too small");

#define CPU(wrap) (&((qe6502_cpuwrap_t*)(wrap))->cpu)
#define CPU_CONST(wrap) (&((const qe6502_cpuwrap_t*)(wrap))->cpu)

QE_FFI_API_IMPL(uint32_t)
qe6502_version(void)
{
    uint16_t version;
    qe6502_version_impl(&version, QE_NULL, QE_NULL);
    return (uint32_t)version;
}

QE_FFI_API_IMPL(size_t)
qe6502_size(void)
{
    return QE6502_CONTEXT_SIZE;
}

QE_FFI_API_IMPL(size_t)
qe6502_align(void)
{
    return QE6502_CONTEXT_ALIGN;
}

//

QE_FFI_API_IMPL(qe6502_tick_t)
qe6502_reset(qe6502_cpu_t* cpu, uint8_t model)
{
    qe6502_cpuwrap_t* wrap = (qe6502_cpuwrap_t*)cpu;
    wrap->cycle = qe6502_reset_impl(&wrap->cpu, model);
    return qe6502_get_tick_impl( &wrap->cpu);
}


QE_FFI_API_IMPL(uint32_t)
qe6502_tick(qe6502_cpu_t* cpu, uint8_t data_in)
{
    qe6502_t* cpu_ptr = CPU(cpu);
    qe6502_cpuwrap_t* wrap = (qe6502_cpuwrap_t*)cpu;

    if( qe6502_needs_data_impl(cpu_ptr) )
    {
        qe6502_feed_data_impl(cpu_ptr, data_in);
    }
    if (QE_LIKELY(wrap->cycle.execute != QE_NULL))
    {
        wrap->cycle = wrap->cycle.execute(&wrap->cpu);
    }
    else
    {
        qe_log_error("NULL cycle handler");
        wrap->cycle = cpu_error(cpu_ptr,  qe6502_err_corrupt_state);
    }

    return qe6502_get_tick_impl( cpu_ptr );
}

QE_FFI_API_IMPL(qe6502_tick_t) qe6502_set_nmi(qe6502_cpu_t* cpu, uint8_t high)
{
    if (high)
    {
        qe6502_nmi_hi_impl(CPU(cpu));
    }
    else
    {
        qe6502_nmi_lo_impl(CPU(cpu));
    }

    return qe6502_get_tick_impl( CPU(cpu) );
}

QE_FFI_API_IMPL(qe6502_tick_t) qe6502_set_irq(qe6502_cpu_t* cpu, uint8_t high)
{
    if (high)
    {
        qe6502_irq_hi_impl(CPU(cpu));
    }
    else
    {
        qe6502_irq_lo_impl(CPU(cpu));
    }

    return qe6502_get_tick_impl( CPU(cpu) );
}

QE_FFI_API_IMPL(uint32_t)
qe6502_last_tickt(qe6502_cpu_t* cpu)
{
    return qe6502_get_tick_impl( CPU(cpu) );
}

//

/*
 * Returns a packed snapshot of the visible CPU registers.
 *
 * Bit layout:
 *   bits [ 0..15] : PC
 *   bits [16..23] : A
 *   bits [24..31] : X
 *   bits [32..39] : Y
 *   bits [40..47] : S
 *   bits [48..55] : P
 *   bits [56..58] : CPU Model
 *   bits [59    ] : Reserved, used internally
 *   bits [60    ] : 0 == NMI pin low | 1 == NMI pin high
 *   bits [61    ] : 0 == IRQ pin low | 1 == IRQ pin high
 *   bits [62    ] : 0 == Running | 1 == Waiting (WAI)
 *   bits [63    ] : 0 == CPU is stable/serializable | 1 == CPU not stable/not serializable/stopped
 *
 * This is a numeric bit encoding. Decode with shifts and masks.
 */
QE_FFI_API_IMPL(uint64_t) qe6502_dump_state(const qe6502_cpu_t* cpu)
{
    const qe6502_t* cpu_ptr = CPU_CONST(cpu);
    qe_word64_t state64;
    state64.word16_0    = cpu_ptr->PC;
    state64.u8_2        = cpu_ptr->A;
    state64.u8_3        = cpu_ptr->X;
    state64.u8_4        = cpu_ptr->Y;
    state64.u8_5        = cpu_ptr->S;
    state64.u8_6        = cpu_ptr->P;
    state64.u8_7        = cpu_ptr->istate; // model and istate shared the same byte

    if (!qe6502_instr_done_impl(cpu_ptr))
    {
        state64.u8_7 |= 1 << 7;
    }
    return state64.u64;
}

QE_FFI_API_IMPL(qe6502_tick_t) qe6502_recover(qe6502_cpu_t* cpu, uint64_t stable_state)
{
    qe6502_t* cpu_ptr = CPU(cpu);

    qe6502_overwrite_impl(cpu_ptr, stable_state);

    qe6502_cpuwrap_t* wrap = (qe6502_cpuwrap_t*)cpu;

    if (stable_state >> 63)
    {
        qe_log_error("CPU cannot be recovered from unstable or halted state");
        wrap->cycle = cpu_error(cpu_ptr,  qe6502_err_resume_error);
    }
    else
    {
        wrap->cycle = reset_to_normal_state(&wrap->cpu);
        qe_log_info("CPU Recovered");
    }
    return qe6502_get_tick_impl(cpu_ptr);
}

QE_FFI_API_IMPL(void) qe6502_unsafe_state_overwrite(qe6502_cpu_t* cpu, qe6502_state_t state)
{
    // clear unstable flag from dump function
    state &= ~((uint64_t)1 << 63);
    qe6502_t* cpu_ptr = CPU(cpu);
    qe6502_overwrite_impl(cpu_ptr, state);
}

QE_FFI_API_IMPL(uint16_t)  qe6502_error_code(const qe6502_cpu_t* cpu)
{
    if (qe6502_ok_impl(CPU_CONST(cpu)))
    {
        return 0;
    }
    return CPU_CONST(cpu)->error_code;
}

QE_FFI_API_IMPL(const char*) qe6502_error_string(const qe6502_cpu_t* cpu)
{
    return qe6502_decode_error( qe6502_error_code(cpu) );
}

QE_FFI_API_IMPL(const char*)  qe6502_decode_error(uint16_t error_code)
{
    switch(error_code)
    {
        case 0: return "No error";
        case 1: return "qe6502 illegal instruction error";
        case 2: return "qe6502 compile error";
        case 3: return "qe6502 logic error";
        case 4: return "qe6502 unknown model error";
        case 5: return "qe6502 boot error";
        case 6: return "qe6502 corrupt_state error";
        case 7: return "qe6502 resume error";
        default:
          break;
    }
    return "Unknown error!";
}

QE_FFI_API_IMPL(void) qe6502_set_logger(qe6502_log_fn logger, void* context)
{
    qe_set_logger((qe_log_fn)logger, context);
}
