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

#include <qe6502/qe6502.h>
#include "qe6502_inline.h"

#if defined(QE6502_ENABLE_DEBUG_LOG) && (QE6502_ENABLE_DEBUG_LOG == 1)
    #include <stdio.h>
    #include <stdarg.h>
    #include <time.h>

#if defined(__GNUC__) || defined(__clang__)
    #define QE_PRINTF_LIKE(fmt_index, va_index) __attribute__((format(printf, fmt_index, va_index)))
#else
    #define QE_PRINTF_LIKE(fmt_index, va_index)
#endif
    QE_INTERNAL_API(void) qe_log(const char* topic, const char *fmt, ...) QE_PRINTF_LIKE(2, 3);

    QE_INTERNAL_API(void) qe_log(const char* topic, const char *fmt, ...)
    {
        /* HH:MM:SS timestamp */
        char timeBuf[9];
        time_t now = time(NULL);
        struct tm tm_now;

    #ifdef _MSC_VER
        localtime_s(&tm_now, &now);
    #else
        localtime_r(&now, &tm_now);
    #endif
        strftime(timeBuf, sizeof timeBuf, "%H:%M:%S", &tm_now);

        printf("[%s | %s] ", timeBuf, topic);

        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);

        putchar('\n');
        fflush(stdout);
    }
#endif

QE_INTERNAL_API(void)
qe6502_version_impl(uint16_t* version,
                    uint8_t* version_major,
                    uint8_t* version_minor)
{
    if (version)        *version = QE6502_VERSION;
    if (version_major)  *version_major = QE6502_VERSION_MAJOR;
    if (version_minor)  *version_minor = QE6502_VERSION_MINOR;
}

/********************************************************
 *
 *  Power on sequence
 *
 ********************************************************/

#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    QE_INTERNAL_API(qe6502_cycle_t) mos_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_INTERNAL_API(qe6502_cycle_t) nes_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    QE_INTERNAL_API(qe6502_cycle_t) rw_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_INTERNAL_API(qe6502_cycle_t) wdc_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    QE_INTERNAL_API(qe6502_cycle_t) st_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif

static qe6502_cycle_t select_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    #if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
        if(qe6502_mos == qe6502_model_impl(cpu))   return mos_fetch_opcode_bridge(cpu);
        if(qe6502_nes == qe6502_model_impl(cpu))   return nes_fetch_opcode_bridge(cpu);
    #endif
    #if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
        if(qe6502_wdc == qe6502_model_impl(cpu))   return wdc_fetch_opcode_bridge(cpu);
        if(qe6502_rw == qe6502_model_impl(cpu))    return rw_fetch_opcode_bridge(cpu);
        if(qe6502_st == qe6502_model_impl(cpu))    return st_fetch_opcode_bridge(cpu);
    #endif
    qe_log("qe6502", "Error: unknown model: %d", (unsigned)(qe6502_model(cpu)));
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
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_lsb));
        cpu->cmd.flags = qe6502_starting;
        return resume_to(power_on_routine);

    case 7:
        cpu->address.u16++;
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_msb));
        cpu->cmd.flags = qe6502_starting;
        return resume_to(power_on_routine);

    case 8:
        return select_fetch_opcode_bridge(cpu);

    default:
        break;
    }
    return cpu_error(cpu,  qe6502_err_boot_error );
}
QE_INTERNAL_API(qe6502_cycle_t)
qe6502_power_on_impl(qe6502_t* cpu, uint8_t model)
{
    qe_log("qe6502", "Power ON");
    qe_log("qe6502", "CPU size: %u", (unsigned)sizeof(qe6502_t));
    qe_log("qe6502", "CPU Cycle size: %u", (unsigned)sizeof(qe6502_cycle_t));
    QE_CLEAR_OBJ(*cpu);

    // test read
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (cpu->cmd.address != 0xDEAD ||
        cpu->cmd.offset != 0xA2)
    {
        qe_log("qe6502", "Error: request_read");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test stack read
    cpu->cmd.packed = 0;
    request_stack_read(cpu, 0xDE, 0xAD);
    if (cpu->cmd.address != 0x01DE ||
        cpu->cmd.offset != 0xAD)
    {
        qe_log("qe6502", "Error: request_stack_read");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test write
    cpu->cmd.packed = 0;
    request_write(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (cpu->cmd.address != 0xDEAD ||
        cpu->cmd.offset != 0xA2 ||
        cpu->cmd.flags != qe6502_writing)
    {
        qe_log("qe6502", "Error: request_write");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test stack write
    cpu->cmd.packed = 0;
    request_stack_write(cpu, 0xDE, 0xAD);
    if (cpu->cmd.address != 0x01DE ||
        cpu->cmd.offset != 0xAD ||
        cpu->cmd.flags != qe6502_writing)
    {
        qe_log("qe6502", "Error: request_stack_write");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // test offsets
    ((uint8_t*)(cpu))[ OFFSETOF(address.u8_lsb) ] = 0xAD;
    ((uint8_t*)(cpu))[ OFFSETOF(address.u8_msb) ] = 0xDE;
    ((uint8_t*)(cpu))[ OFFSETOF(pointer.u8_lsb) ] = 0xEF;
    ((uint8_t*)(cpu))[ OFFSETOF(pointer.u8_msb) ] = 0xBE;
    ((uint8_t*)(cpu))[ OFFSETOF(PC.u8_lsb) ] = 0xFE;
    ((uint8_t*)(cpu))[ OFFSETOF(PC.u8_msb) ] = 0xCA;
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
        qe_log("qe6502", "Error: CPU internal offsets");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // endian check
    cpu->address.u8_msb = 0x79;
    cpu->address.u8_lsb = 0x09;
    if (cpu->address.u16 != 0x7909)
    {
        qe_log("qe6502", "Error: little/big endian check");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    cpu->cmd.packed = 0xC001BABE;
    if (cpu->cmd.address != 0xBABE ||
        cpu->cmd.offset != 0x01 ||
        cpu->cmd.flags != 0xC0)
    {
        qe_log("qe6502", "Error: little/big endian 32 check");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    // istate
    cpu->istate = 0;

    // check model
    if ( (qe6502_model_max + 1) & qe6502_model_max )
    {
        qe_log("qe6502", "Error: qe6502_model_max shoud be mask");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    if (qe6502_model_max >= qe6502_nmi_pin_chg ||
        qe6502_model_max >= qe6502_nmi_pin_lo  ||
        qe6502_model_max >= qe6502_irq_pin_lo )
    {
        qe_log("qe6502", "Error: istate pin values must be greater than qe6502_model_max");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    if (model > qe6502_model_max)
    {
        qe_log("qe6502", "Error: Model id too big");
        return cpu_error(cpu,  qe6502_err_unknown_model );
    }
    cpu->model = model;
#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    if(qe6502_mos == qe6502_model_impl(cpu))   qe_log("qe6502", "Model set to 'MOS 6502'");       else
    if(qe6502_nes == qe6502_model_impl(cpu))   qe_log("qe6502", "Model set to 'NES 6502'");       else
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    if(qe6502_wdc == qe6502_model_impl(cpu))   qe_log("qe6502", "Model set to 'WDC 65C02'");      else
    if(qe6502_rw == qe6502_model_impl(cpu))    qe_log("qe6502", "Model set to 'Rockwell 65C02'"); else
    if(qe6502_st == qe6502_model_impl(cpu))    qe_log("qe6502", "Model set to 'Synertek 65C02'"); else
#endif
    {
        qe_log("qe6502", "Error: unknown model: %d", (unsigned)model);
        return cpu_error(cpu,  qe6502_err_unknown_model );
    }

    // test API
    cpu->cmd.flags = qe6502_halted;
    if (qe6502_ok_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_ok' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = 0;
    if (!qe6502_ok_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_ok' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (!qe6502_needs_data_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_needs_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_has_data_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_has_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_write(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (qe6502_needs_data_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_needs_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (!qe6502_has_data_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_has_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_address_impl(cpu) != 0xDEAD)
    {
        qe_log("qe6502", "Error: 'qe6502_address' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = qe6502_wait_opcode;
    if (!qe6502_instr_done_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_instr_done' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = 0;
    if (qe6502_instr_done_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_instr_done' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word_t){.u16=0xBEEF}, OFFSETOF(data));
    qe6502_feed_data_impl(cpu, 0x42);
    if (cpu->data != 0x42)
    {
        qe_log("qe6502", "Error: 'qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    request_write(cpu, (qe_word_t){.u16=0xBEEF}, OFFSETOF(data));
    if (qe6502_data_impl(cpu) != 0x42)
    {
        qe_log("qe6502", "Error: 'qe6502_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_cycle_t cycle = qe6502_reset_instruction_impl(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok_impl(cpu) ||
        !qe6502_instr_done_impl(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_reset_instruction' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_feed_data_impl(cpu, 0xEA);
    if (cpu->opcode != 0xEA)
    {
        qe_log("qe6502", "Error: 'qe6502_reset_instruction or qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cycle = cycle.execute(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok_impl(cpu))
    {
        qe_log("qe6502", "Error: 'internal cycle exec' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }

    qe_log("qe6502", "Self-test OK");

#if(QE6502_ENABLE_CYCLE_MERGE == 1)
    qe_log("qe6502", "Warning: Cycles merge enabled!");
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

// Foreign Function Interface (FFI)

typedef struct qe6502_cpuwrap
{
    qe6502_t cpu;
    qe6502_cycle_t cycle;
} qe6502_cpuwrap_t;

#define CPU(wrap) (&((qe6502_cpuwrap_t*)(wrap))->cpu)
#define CPU_CONST(wrap) (&((const qe6502_cpuwrap_t*)(wrap))->cpu)

QE_FFI_API_IMPL(uint32_t)
qe6502_version(void)
{
    qe_word32_t w;
    qe6502_version_impl(&w.msw.u16, &w.lsw.u8_msb, &w.lsw.u8_lsb);
    return w.u32;
}

QE_FFI_API_IMPL(size_t)
qe6502_cpu_size(void)
{
    return sizeof(qe6502_cpuwrap_t);
}

//

QE_FFI_API_IMPL(size_t)
qe6502_cpu_create(void* cpu_memory, size_t memory_size)
{
    if (QE_NULL == cpu_memory)
    {
        qe_log("qe6502", "Error: memory buffer NULL, must be %zu bytes valid buffer", qe6502_cpu_size());
        return 0;
    }
    if (memory_size < qe6502_cpu_size())
    {
        qe_log("qe6502", "Error: memory buffer too small, must be %zu bytes", qe6502_cpu_size());
        return 0;
    }
    qe_memset(cpu_memory, 0, qe6502_cpu_size());
    return qe6502_cpu_size();
}

QE_FFI_API_IMPL(void)
qe6502_cpu_power_on(void* cpu, uint8_t model)
{
    qe6502_cpuwrap_t* wrap = (qe6502_cpuwrap_t*)cpu;
    wrap->cycle = qe6502_power_on_impl(&wrap->cpu, model);
}

QE_FFI_API_IMPL(void)
qe6502_cpu_tick(void* cpu)
{
    qe6502_cpuwrap_t* wrap = (qe6502_cpuwrap_t*)cpu;
    wrap->cycle = wrap->cycle.execute(&wrap->cpu);
}

//

QE_FFI_API_IMPL(uint8_t)  qe6502_ok(const void* cpu) { return qe6502_ok_impl(CPU(cpu));}
QE_FFI_API_IMPL(uint8_t)  qe6502_needs_data(const void* cpu) { return qe6502_needs_data_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(uint8_t)  qe6502_has_data(const void* cpu) { return qe6502_has_data_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(void)     qe6502_feed_data(void* cpu, uint8_t byte) { qe6502_feed_data_impl(CPU(cpu), byte);}
QE_FFI_API_IMPL(uint8_t)  qe6502_data(const void* cpu) { return qe6502_data_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(uint16_t) qe6502_address(const void* cpu) { return qe6502_address_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(uint8_t)  qe6502_instr_done(const void* cpu) { return qe6502_instr_done_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(uint8_t)  qe6502_started(const void* cpu)
{
    return qe6502_started_impl(CPU_CONST(cpu));
}
QE_FFI_API_IMPL(uint8_t)  qe6502_model(const void* cpu) { return qe6502_model_impl(CPU_CONST(cpu));}

QE_FFI_API_IMPL(uint8_t)  qe6502_nmi_pin(const void* cpu) { return qe6502_nmi_pin_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(void)     qe6502_nmi_hi(void* cpu) { qe6502_nmi_hi_impl(CPU(cpu));}
QE_FFI_API_IMPL(void)     qe6502_nmi_lo(void* cpu) { qe6502_nmi_lo_impl(CPU(cpu));}

QE_FFI_API_IMPL(uint8_t)  qe6502_irq_pin(const void* cpu) { return qe6502_irq_pin_impl(CPU_CONST(cpu));}
QE_FFI_API_IMPL(void)     qe6502_irq_hi(void* cpu) { qe6502_irq_hi_impl(CPU(cpu));}
QE_FFI_API_IMPL(void)     qe6502_irq_lo(void* cpu) { qe6502_irq_lo_impl(CPU(cpu));}

QE_FFI_API_IMPL(uint16_t)  qe6502_error_code(const void* cpu)
{
    if (qe6502_ok_impl(CPU_CONST(cpu)))
    {
        return 0;
    }
    return CPU(cpu)->error_code;
}

QE_FFI_API_IMPL(const char*)  qe6502_error_string(uint16_t error_code)
{
    switch(error_code)
    {
        case 0: return "";
        case (1 << 0): return "qe6502_err_compile_error  ";
        case (1 << 1): return "qe6502_err_illegal_instr  ";
        case (1 << 2): return "qe6502_err_poweron_error  ";
        case (1 << 3): return "qe6502_err_logic_error    ";
        case (1 << 4): return "qe6502_err_unknown_model  ";
        case (1 << 6): return "qe6502_err_boot_error     ";
        case (1 << 7): return "qe6502_err_interrupt_error";
        default:
          break;
    }
    return "Unknowo error!";
}

QE_FFI_API_IMPL(void)
qe6502_offsets(qe6502_offsets_t* offsets)
{
    offsets->word_lsb           = offsetof(qe_word_t, u8_lsb);
    offsets->word_msb           = offsetof(qe_word_t, u8_msb);
    offsets->word32_lsw         = offsetof(qe_word32_t, lsw);
    offsets->word32_msw         = offsetof(qe_word32_t, msw);
    offsets->cmd_address        = OFFSETOF(cmd) + offsetof(qe6502_cmd_t, address);
    offsets->cmd_offset         = OFFSETOF(cmd) + offsetof(qe6502_cmd_t, offset);
    offsets->cmd_flags          = OFFSETOF(cmd) + offsetof(qe6502_cmd_t, flags);
    offsets->cmd_packed         = OFFSETOF(cmd) + offsetof(qe6502_cmd_t, packed);
    offsets->address            = OFFSETOF(address);
    offsets->pointer            = OFFSETOF(pointer);
    offsets->error_code         = OFFSETOF(error_code);
    offsets->instr              = OFFSETOF(instr);
    offsets->merged             = OFFSETOF(merged);
    offsets->data               = OFFSETOF(data);
    offsets->pagecross_overflow = OFFSETOF(pagecross_overflow);
    offsets->model              = OFFSETOF(model);
    offsets->istate             = OFFSETOF(istate);
    offsets->opcode             = OFFSETOF(opcode);
    offsets->PC                 = OFFSETOF(PC);
    offsets->S                  = OFFSETOF(S);
    offsets->A                  = OFFSETOF(A);
    offsets->X                  = OFFSETOF(X);
    offsets->Y                  = OFFSETOF(Y);
    offsets->P                  = OFFSETOF(P);

    qe6502_t obj;
    offsets->sizeof_word                = sizeof(qe_word_t);
    offsets->sizeof_word32              = sizeof(qe_word32_t);
    offsets->sizeof_cmd_address         = sizeof(obj.cmd.address);
    offsets->sizeof_cmd_offset          = sizeof(obj.cmd.offset);
    offsets->sizeof_cmd_flags           = sizeof(obj.cmd.flags);
    offsets->sizeof_cmd_packed          = sizeof(obj.cmd.packed);
    offsets->sizeof_address             = sizeof(obj.address);
    offsets->sizeof_pointer             = sizeof(obj.pointer);
    offsets->sizeof_error_code          = sizeof(obj.error_code);
    offsets->sizeof_instr               = sizeof(obj.instr);
    offsets->sizeof_merged              = sizeof(obj.merged);
    offsets->sizeof_data                = sizeof(obj.data);
    offsets->sizeof_pagecross_overflow  = sizeof(obj.pagecross_overflow);
    offsets->sizeof_model               = sizeof(obj.model);
    offsets->sizeof_istate              = sizeof(obj.istate);
    offsets->sizeof_opcode              = sizeof(obj.opcode);
    offsets->sizeof_PC                  = sizeof(obj.PC);
    offsets->sizeof_S                   = sizeof(obj.S);
    offsets->sizeof_A                   = sizeof(obj.A);
    offsets->sizeof_X                   = sizeof(obj.X);
    offsets->sizeof_Y                   = sizeof(obj.Y);
    offsets->sizeof_P                   = sizeof(obj.P);
}
