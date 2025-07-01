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

#include "6502_inline.h"

QE_API_IMPL
void qe6502_version(uint16_t* version,
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

#if (QE6502_ENABLE_NMOS_6502 == 1)
    qe6502_cycle_t mos_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    qe6502_cycle_t nes_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif
#if (QE6502_ENABLE_CMOS_65C02 == 1)
    qe6502_cycle_t rw_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    qe6502_cycle_t wdc_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
    qe6502_cycle_t st_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu );
#endif

qe6502_cycle_t select_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    #if (QE6502_ENABLE_NMOS_6502 == 1)
        if(qe6502_mos == cpu->model)   return mos_fetch_opcode_bridge(cpu);
        if(qe6502_nes == cpu->model)   return nes_fetch_opcode_bridge(cpu);
    #endif
    #if (QE6502_ENABLE_CMOS_65C02 == 1)
        if(qe6502_wdc == cpu->model)   return wdc_fetch_opcode_bridge(cpu);
        if(qe6502_rw == cpu->model)    return rw_fetch_opcode_bridge(cpu);
        if(qe6502_st == cpu->model)    return st_fetch_opcode_bridge(cpu);
    #endif
    qe_log("qe6502", "Error: unknown model: %d", (unsigned)cpu->model);
    return cpu_error(cpu,  qe6502_err_unknown_model );
}

INSTR_RETTYPE qe6502_cycle_t
power_on_impl( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->opcode++;
    cpu->P = (1 << 5) | (1 << 2);
    switch (cpu->opcode)
    {
    case 2:
        cpu->PC.u16++;
        request_read(cpu, cpu->PC, OFFSETOF(data));
        cpu->cmd.flags = qe6502_starting;

        return resume_to(power_on_impl);
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

        return resume_to(power_on_impl);
    case 6:
        cpu->address.u16 = 0xFFFC;
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_lsb));
        cpu->cmd.flags = qe6502_starting;

        return resume_to(power_on_impl);
    case 7:
        cpu->address.u16++;
        request_read(cpu, cpu->address, OFFSETOF(PC.u8_msb));
        cpu->cmd.flags = qe6502_starting;

        return resume_to(power_on_impl);
    default:

        return select_fetch_opcode_bridge(cpu);
    }
    QE_UNREACHABLE();
}
QE_API_IMPL qe6502_cycle_t
qe6502_power_on(qe6502_t* cpu, uint8_t model)
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

    // check model
    cpu->model = model;
#if (QE6502_ENABLE_NMOS_6502 == 1)
    if(qe6502_mos == cpu->model)   qe_log("qe6502", "Model set to 'MOS 6502'");       else
    if(qe6502_nes == cpu->model)   qe_log("qe6502", "Model set to 'NES 6502'");       else
#endif
#if (QE6502_ENABLE_CMOS_65C02 == 1)
    if(qe6502_wdc == cpu->model)   qe_log("qe6502", "Model set to 'WDC 65C02'");      else
    if(qe6502_rw == cpu->model)    qe_log("qe6502", "Model set to 'Rockwell 65C02'"); else
    if(qe6502_st == cpu->model)    qe_log("qe6502", "Model set to 'Synertek 65C02'"); else
#endif
    {
        qe_log("qe6502", "Error: unknown model: %d", (unsigned)model);
        return cpu_error(cpu,  qe6502_err_unknown_model );
    }

    // test API
    cpu->cmd.flags = qe6502_halted;
    if (qe6502_ok(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_ok' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = 0;
    if (!qe6502_ok(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_ok' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (!qe6502_needs_data(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_needs_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_has_data(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_has_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_write(cpu, (qe_word_t){.u16=0xDEAD}, 0xA2);
    if (qe6502_needs_data(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_needs_data' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (!qe6502_has_data(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_has_data' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    if (qe6502_address(cpu) != 0xDEAD)
    {
        qe_log("qe6502", "Error: 'qe6502_address' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = qe6502_wait_opcode;
    if (!qe6502_instr_done(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_instr_done' wrong false");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.flags = 0;
    if (qe6502_instr_done(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_instr_done' wrong true");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cpu->cmd.packed = 0;
    request_read(cpu, (qe_word_t){.u16=0xBEEF}, OFFSETOF(data));
    qe6502_feed_data(cpu, 0x42);
    if (cpu->data != 0x42)
    {
        qe_log("qe6502", "Error: 'qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    request_write(cpu, (qe_word_t){.u16=0xBEEF}, OFFSETOF(data));
    if (qe6502_data(cpu) != 0x42)
    {
        qe_log("qe6502", "Error: 'qe6502_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_cycle_t cycle = qe6502_reset_instruction(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok(cpu) ||
        !qe6502_instr_done(cpu))
    {
        qe_log("qe6502", "Error: 'qe6502_reset_instruction' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    qe6502_feed_data(cpu, 0xEA);
    if (cpu->opcode != 0xEA)
    {
        qe_log("qe6502", "Error: 'qe6502_reset_instruction or qe6502_feed_data' fail");
        return cpu_error(cpu,  qe6502_err_compile_error );
    }
    cycle = cycle.execute(cpu);
    if (QE_NULL == cycle.execute ||
        !qe6502_ok(cpu))
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
    cpu->model = model;

    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->cmd.flags = qe6502_starting;
    cpu->opcode = 1;
    return resume_to(power_on_impl);
}

QE_API_IMPL qe6502_cycle_t
qe6502_reset_instruction(qe6502_t *cpu)
{
    cpu->P |= qe6502_flag_UN;

    return select_fetch_opcode_bridge(cpu);
}

QE_API_IMPL void
qe6502_offsets(qe6502_offsets_t* offsets)
{
    offsets->address            = OFFSETOF(address);
    offsets->pointer            = OFFSETOF(pointer);
    offsets->error_code         = OFFSETOF(error_code);
    offsets->data               = OFFSETOF(data);
    offsets->pagecross_overflow = OFFSETOF(pagecross_overflow);
    offsets->opcode             = OFFSETOF(opcode);
    offsets->PC                 = OFFSETOF(PC);
    offsets->S                  = OFFSETOF(S);
    offsets->A                  = OFFSETOF(A);
    offsets->X                  = OFFSETOF(X);
    offsets->Y                  = OFFSETOF(Y);
    offsets->P                  = OFFSETOF(P);
}



