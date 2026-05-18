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

#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)

#define QE6502_CMOS_OPCODE_UNLOCKER
#   include "qe6502_cmos_opcodes.h"
#undef QE6502_CMOS_OPCODE_UNLOCKER

#include <qe/log.h>

/********************************************************
 *
 *  Start new instruction
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_opcode_dispatcher( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC.u16++;
    cpu->instr = cmos_model.opcodes[cpu->opcode].instr;
    return jump_to(cpu,  cmos_model.opcodes[cpu->opcode].entry );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_fetch_opcode( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(opcode));

    if (cpu->istate & ( qe6502_nmi_pin_chg | qe6502_irq_pin_lo ))
    {
        if (cpu->istate & qe6502_nmi_pin_chg)
        {
            cpu->address.u8_0 = 0;
            return resume_to_dummy_read( cpu, cmos_nmi );
        }

        if (  (cpu->istate & qe6502_irq_pin_lo) &&
              (!(cpu->P & qe6502_flag_I)) )
        {
            cpu->address.u8_0 = 0;
            return resume_to_dummy_read( cpu, cmos_irq );
        }
    }

    cpu->cmd.flags |= qe6502_wait_opcode;

    return resume_to( cmos_opcode_dispatcher );
}

QE_INTERNAL_API(qe6502_cycle_t)
rw_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    qe_log_info("Rockwell fetcher attached");
    return cmos_fetch_opcode(cpu);
}

QE_INTERNAL_API(qe6502_cycle_t)
wdc_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    qe_log_info("WDC fetcher attached");
    return cmos_fetch_opcode(cpu);
}

QE_INTERNAL_API(qe6502_cycle_t)
st_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    qe_log_info("Synertek fetcher attached");
    return cmos_fetch_opcode(cpu);
}

////////////////////////////////////////////////////////////////////////

INSTR_FW_DECL(cmos_do_relative_branch_2);

INSTR_RETTYPE qe6502_cycle_t
cmos_do_relative_branch( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    qe_word16_t pc;
    pc.u16 = QE_U16(cpu->PC.u16 + cpu->address.i8_0);
    cpu->PC.u8_0 = pc.u8_0;
    if (pc.u8_1 == cpu->PC.u8_1)
    {
        // no page crossing
        return resume_to(cmos_fetch_opcode);
    }
    cpu->address = pc;
    return resume_to_dummy_read(cpu, cmos_do_relative_branch_2);

}

INSTR_RETTYPE qe6502_cycle_t
cmos_do_relative_branch_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC = cpu->address;
    return resume_to(cmos_fetch_opcode);
}

////////////////////////////////////////////////////////////////////////

/********************************************************
 *                      ADC
 *
 *  Description: Add with Carry
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x61( 97)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0x65(101)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0x69(105)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0x6D(109)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0x71(113)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0x75(117)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0x79(121)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0x7D(125)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ADC_impl_bin( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t u = cpu->data;
    qe_word16_t r16 = (qe_word16_t){.u16 = cpu->A};

    uint8_t carry = QE_U8(QE_U8(cpu->P & qe6502_flag_C ) >> QE_U8(qe6502_flagpos_C));
    r16.u16 += QE_U16(u + carry);
    uint8_t c = r16.u16 > 0xff;

    uint8_t flags = QE_U8(
                    (c) |  // carry
                    ((r16.u8_0 == 0) << qe6502_flagpos_Z) | // zero
                    (r16.u8_0 & qe6502_flag_N) |  // negative
                    (((((~(cpu->A ^ u)) & (cpu->A ^ r16.u8_0)) & qe6502_flag_N) != 0) << qe6502_flagpos_V)); // overflow

    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    cpu->A = r16.u8_0;

    return jump_to(cpu, cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ADC_impl_dec( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t data = cpu->data;
    uint8_t carry = QE_U8(QE_U8(cpu->P & qe6502_flag_C ) >> QE_U8(qe6502_flagpos_C));
    uint16_t result = 0;
    uint8_t flags = 0;

    result = QE_U16((cpu->A & 0x0f) + (data & 0x0f) + carry);
    if (result >= 0x0A)
    {
        result = 0x10 | ((result + 6) & 0x0f);
    }
    result += QE_U16((cpu->A & 0xf0) + (data & 0xf0));

    if (result >= 0xA0)
    {
        flags |= qe6502_flag_C;
        flags |= QE_U8((result < 0x180) ? (!((cpu->A ^ data) & 0x80)) << qe6502_flagpos_V : 0);
        result += 0x60;
    }
    else
    {
        flags |= QE_U8((result >= 0x80) ? (!((cpu->A ^ data) & 0x80)) << qe6502_flagpos_V : 0);
    }
    flags |= QE_U8((((uint8_t)result == 0) << qe6502_flagpos_Z));
    flags |= QE_U8(result & qe6502_flag_N);
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    //
    cpu->A = (uint8_t)result;
    return jump_to(cpu, cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SBC_impl_bin( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data ^= 0xff;
    return jump_to(cpu, cmos_instr_ADC_impl_bin);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SBC_impl_dec( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t data = cpu->data;
    uint16_t high = 0;
    uint8_t low = 0;
    uint8_t flags = 0;

    uint8_t carry = QE_U8((cpu->P & qe6502_flag_C ) >> qe6502_flagpos_C);
    low = QE_U8(0x0F + (cpu->A & 0x0F) - (data & 0x0F) + carry);
    high = (cpu->A & 0xF0) - (data & 0xF0);

    if (low < 0x10)
    {
        low -= 0x06;
    }
    else
    {
        high += 0x10; // carry
        low -= 0x10;
    }
    if ( ((cpu->A ^ data) & (cpu->A ^ ((high&0xf0)+(low&0x0f)-0x10))) & qe6502_flag_N )
    {
        flags |= qe6502_flag_V;
    }
    high += 0xF0;
    if (high < 0x100)
    {
        high -= 0x60;
    }
    else
    {
        flags |= qe6502_flag_C;
    }

    uint8_t result = (uint8_t)(high + low);
    flags |= QE_U8(((uint8_t)result == 0) << qe6502_flagpos_Z);
    flags |= QE_U8(result & qe6502_flag_N);
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    //
    cpu->A = result;
    return jump_to(cpu, cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ADC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, cmos_instr_ADC_impl_bin);
    }
    else
    {
        request_read(cpu, cpu->address, OFFSETOF(pointer.u8_0));
        return resume_to(cmos_instr_ADC_impl_dec);
    }
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ADC_Immediate( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, cmos_instr_ADC_impl_bin);
    }
    else
    {
        // 0x0059 Rockwell, 0x007f wdc
        uint8_t model = qe6502_model_impl(cpu);
        qe_word16_t dummy_addres =
                model==qe6502_rw  ?(qe_word16_t){.u16 = 0x0059}:
                model==qe6502_wdc ?(qe_word16_t){.u16 = 0x007f}:
                model==qe6502_st  ?(qe_word16_t){.u16 = 0x0056}:
                                   (qe_word16_t){.u16 = 0x0000};
        request_read(cpu, dummy_addres, OFFSETOF(pointer.u8_0));
        return resume_to(cmos_instr_ADC_impl_dec);
    }
}

/********************************************************
 *                      AND
 *
 *  Description: Logical AND
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x21( 33)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0x25( 37)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0x29( 41)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0x2D( 45)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0x31( 49)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0x35( 53)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0x39( 57)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0x3D( 61)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_AND( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A &= cpu->data;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                        (cpu->A?0:qe6502_flag_Z)|
                        (cpu->A & qe6502_flag_N));
    // now
    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      ASL
 *
 *  Description: Arithmetic Shift Left
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x06(  6)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x0A( 10)  Bytes: 1,  Mode: Accumulator
 *          See also:  cmos_pre_rw_accumulator
 *
 *  Opcode: 0x0E( 14)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0x16( 22)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x1E( 30)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ASL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data >> 7;
    cpu->data = QE_U8(cpu->data << 1);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                        carry|
                        (cpu->data?0:qe6502_flag_Z)|
                        (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ASL_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A >> 7;
    cpu->A = QE_U8(cpu->A << 1);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                        carry|
                        (cpu->A?0:qe6502_flag_Z)|
                        (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      BCC
 *
 *  Description: Branch if Carry Clear
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x90(144)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BCC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_C)
    {
        // carry not clear
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BCS
 *
 *  Description: Branch if Carry Set
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xB0(176)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BCS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_C))
    {
        // carry not set
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BEQ
 *
 *  Description: Branch if Equal
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xF0(240)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BEQ( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_Z))
    {
        // zero is clear
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BIT
 *
 *  Description: Bit Test
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x24( 36)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0x2C( 44)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BIT( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{

    update_flags(cpu, qe6502_flag_Z | qe6502_flag_V | qe6502_flag_N,
                            QE_U8(
                            ((!(cpu->A & cpu->data)) << qe6502_flagpos_Z) |
                            (cpu->data & (qe6502_flag_V | qe6502_flag_N))));

    return jump_to(cpu, cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BIT_immediate( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z,
                        QE_U8(
                        (!(cpu->A & cpu->data)) << qe6502_flagpos_Z));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      BMI
 *
 *  Description: Branch if Minus
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x30( 48)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BMI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_N))
    {
        // negative is clear
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BNE
 *
 *  Description: Branch if Not Equal
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xD0(208)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BNE( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_Z)
    {
        // zero is set (equal)
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BPL
 *
 *  Description: Branch if Positive
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x10( 16)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BPL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_N)
    {
        // zero is set (equal)
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BRK
 *
 *  Description: Force Interrupt
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x00(  0)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_BRK_2);
INSTR_FW_DECL(cmos_instr_BRK_finalize);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BRK( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    // we are not clearing our tmp memory, so prepare memory and go to switch-case
    // we will use address lo
    cpu->address.u8_0 = 0;
    return resume_to_dummy_read(cpu, cmos_instr_BRK_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BRK_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_0++;
    switch(cpu->address.u8_0)
    {
    case 1:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_1));
        cpu->S--;
        break;
    case 2:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_0));
        cpu->S--;
        break;
    case 3:
        cpu->data = cpu->P | qe6502_flag_B;
        request_stack_write(cpu, cpu->S, OFFSETOF(data));
        cpu->S--;
        break;
    case 4:
        request_read(cpu, (qe_word16_t){.u16=0xfffe}, OFFSETOF(PC.u8_0));
        break;
    case 5:
        return jump_to(cpu, cpu->instr);
    default:
        qe_log_error("BRK unexpected state");
        return cpu_error(cpu,  qe6502_err_logic_error );
    }
    return resume_to(cmos_instr_BRK_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BRK_finalize( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, (qe_word16_t){.u16=0xffff}, OFFSETOF(PC.u8_1));
    cpu->P |= qe6502_flag_I;
    cpu->P &= QE_U8(~qe6502_flag_D);
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      BVC
 *
 *  Description: Branch if Overflow Clear
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x50( 80)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BVC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_V)
    {
        // overflow set
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      BVS
 *
 *  Description: Branch if Overflow Set
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x70(112)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BVS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_V))
    {
        // overflow clear
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      CLC
 *
 *  Description: Clear Carry Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x18( 24)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CLC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= QE_U8( ~qe6502_flag_C );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CLD
 *
 *  Description: Clear Decimal Mode
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xD8(216)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CLD( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= QE_U8( ~qe6502_flag_D );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CLI
 *
 *  Description: Clear Interrupt Disable
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x58( 88)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CLI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= QE_U8( ~qe6502_flag_I );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CLV
 *
 *  Description: Clear Overflow Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xB8(184)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CLV( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= QE_U8( ~qe6502_flag_V );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CMP
 *
 *  Description: Compare
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xC1(193)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0xC5(197)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xC9(201)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xCD(205)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0xD1(209)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0xD5(213)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0xD9(217)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0xDD(221)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CMP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        (cpu->A >= cpu->data) |
                        ((cpu->A == cpu->data) << qe6502_flagpos_Z) |
                        ((cpu->A - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CPX
 *
 *  Description: Compare X Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xE0(224)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xE4(228)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xEC(236)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CPX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        (cpu->X >= cpu->data) |
                            ((cpu->X == cpu->data) << qe6502_flagpos_Z) |
                            ((cpu->X - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      CPY
 *
 *  Description: Compare Y Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xC0(192)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xC4(196)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xCC(204)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_CPY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            (cpu->Y >= cpu->data) |
                            ((cpu->Y == cpu->data) << qe6502_flagpos_Z) |
                            ((cpu->Y - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      DEC
 *
 *  Description: Decrement Memory
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0xC6(198)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0xCE(206)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0xD6(214)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0xDE(222)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_DEC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_DEC_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->A) << qe6502_flagpos_Z) |
                        (cpu->A & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      DEX
 *
 *  Description: Decrement X Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xCA(202)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_DEX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      DEY
 *
 *  Description: Decrement Y Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x88(136)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_DEY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      EOR
 *
 *  Description: Exclusive OR
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x41( 65)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0x45( 69)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0x49( 73)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0x4D( 77)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0x51( 81)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0x55( 85)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0x59( 89)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0x5D( 93)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_EOR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A ^= cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      INC
 *
 *  Description: Increment Memory
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0xE6(230)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0xEE(238)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0xF6(246)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0xFE(254)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_INC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_INC_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->A) << qe6502_flagpos_Z) |
                        (cpu->A & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      INX
 *
 *  Description: Increment X Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xE8(232)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_INX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      INY
 *
 *  Description: Increment Y Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xC8(200)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_INY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      JMP
 *
 *  Description: Jump
 *
 *  Class: Jmp
 *  Variants:
 *  Opcode: 0x4C( 76)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_jmp_absolute
 *
 *  Opcode: 0x6C(108)  Bytes: 3,  Mode: Indirect
 *          See also:  cmos_pre_jmp_indirect
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JMP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC = cpu->address;
    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      JSR
 *
 *  Description: Jump to Subroutine
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x20( 32)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_sp_absolute
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_JSR_2);
INSTR_FW_DECL(cmos_instr_JSR_3);
INSTR_FW_DECL(cmos_instr_JSR_4);
INSTR_FW_DECL(cmos_instr_JSR_5);
INSTR_FW_DECL(cmos_instr_JSR_6);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    return resume_to(cmos_instr_JSR_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_JSR_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_1));
    cpu->S--;
    return resume_to(cmos_instr_JSR_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_0));
    cpu->S--;
    return resume_to(cmos_instr_JSR_5);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    return resume_to(cmos_instr_JSR_6);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_JSR_6( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC = cpu->address;
    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      LDA
 *
 *  Description: Load Accumulator
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA1(161)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0xA5(165)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xA9(169)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xAD(173)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0xB1(177)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0xB5(181)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0xB9(185)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0xBD(189)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_LDA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));
    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      LDX
 *
 *  Description: Load X Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA2(162)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xA6(166)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xAE(174)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0xB6(182)  Bytes: 2,  Mode: ZeroPage_Y
 *          See also:  cmos_pre_r_zeropage_y
 *
 *  Opcode: 0xBE(190)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_LDX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      LDY
 *
 *  Description: Load Y Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA0(160)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xA4(164)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xAC(172)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0xB4(180)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0xBC(188)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_LDY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));
    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      LSR
 *
 *  Description: Logical Shift Right
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x46( 70)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x4A( 74)  Bytes: 1,  Mode: Accumulator
 *          See also:  cmos_pre_rw_accumulator
 *
 *  Opcode: 0x4E( 78)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0x56( 86)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x5E( 94)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_LSR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data & 1;
    cpu->data >>= 1;
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            carry |
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_LSR_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A & 1;
    cpu->A >>= 1;
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            carry |
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      NOP
 *
 *  Description: No Operation
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0xEA(234)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_NOP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

static const uint8_t illegal_nop_lsb[16] = {
    //  x0    x1  x2   x3  x4  x5   x6  x7   x8   x9  xA   xB  xC  xD   xE  xF
    255, 255,  0,  1,  2, 255, 255,  3, 255, 255, 255, 4,  5, 255, 255, 6
};

// The following table lists the undocumented NOPs of the 65C02.
// You'll see they don't necessarily match the two-byte,
// two-cycle characteristics of the standard NOP (opcode $EA).
// For each entry in the table, the first number is the size in bytes
// and the second number is the number of cycles taken.
// After the second number, a lower-case letter may be present, indicating a footnote.
static const uint8_t illegal_nop_table[16][7][3] = {
    // http://www.6502.org/tutorials/65c02opcodes.html
    //         x2:           x3:          x4:         x7:         xB:         xC:         xF:
    //       -----          -----        -----       -----       -----       -----       -----
    /*0x:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {3,3,'c'}},
    /*1x:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*2x:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*3x:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*4x:*/ {{2,2,'.'},   {1,1,'.'},   {2,3,'g'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*5x:*/ {{0,0,'.'},   {1,1,'.'},   {2,4,'h'},  {1,1,'a'},  {1,1,'.'},  {3,8,'j'},  {1,1,'c'}},
    /*6x:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*7x:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'a'},  {1,1,'.'},  {0,0,'.'},  {1,1,'c'}},
    /*8x:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'.'},  {0,0,'.'},  {1,1,'d'}},
    /*9x:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'.'},  {0,0,'.'},  {1,1,'d'}},
    /*Ax:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'.'},  {0,0,'.'},  {1,1,'d'}},
    /*Bx:*/ {{0,0,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'.'},  {0,0,'.'},  {1,1,'d'}},
    /*Cx:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'e'},  {0,0,'.'},  {1,1,'d'}},
    /*Dx:*/ {{0,0,'.'},   {1,1,'.'},   {2,4,'h'},  {1,1,'b'},  {2,4,'f'},  {3,4,'i'},  {1,1,'d'}},
    /*Ex:*/ {{2,2,'.'},   {1,1,'.'},   {0,0,'.'},  {1,1,'b'},  {1,1,'.'},  {0,0,'.'},  {1,1,'d'}},
    /*Fx:*/ {{0,0,'.'},   {1,1,'.'},   {2,4,'h'},  {1,1,'b'},  {1,1,'.'},  {3,4,'i'},  {1,1,'d'}}
};
/*
a) 1-cycle NOP on some older 65C02s; RMB instruction on Rockwell and on modern WDC 65C02s
b) 1-cycle NOP on some older 65C02s; SMB instruction on Rockwell and on modern WDC 65C02s
c) 1-cycle NOP on some older 65C02s; BBR instruction on Rockwell and on modern WDC 65C02s
d) 1-cycle NOP on some older 65C02s; BBS instruction on Rockwell and on modern WDC 65C02s
e) $CB is the WAI instruction on WDC 65C02
f) $DB is the STP instruction on WDC 65C02
g) $44 uses zp address mode to read memory
h) $54, $D4, and $F4 use zp,X address mode to read memory
i) $DC and $FC use absolute address mode to read memory
j) $5C reads from somewhere in the 64K range, using no known address mode
    * j) I'm not sure whether it should be 8 cycles as stated in the table
    *    or 4 cycles as required in the SingleInstructionTest data.
    * e) I'm not sure whether it should be 1 cycle as stated in the table
    *    or 2 cycles as required in the SingleInstructionTest data (official NOP).
*/

INSTR_FW_DECL(cmos_instr_ILLEGAL_NOP_loop);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ILLEGAL_NOP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t i1 = cpu->opcode >> 4;
    uint8_t i2 = illegal_nop_lsb[ cpu->opcode & 0x0f ];
    if (i2 >= QE_ARRAY_LENGTH(illegal_nop_table[0]))
    {
        qe_log_error("illegal_nop_lsb out of bounds, opcode: %u, idx: %u, value: %u, size: %u",
                (unsigned)cpu->opcode, (unsigned)(cpu->opcode & 0x0f), (unsigned)i2, (unsigned)(QE_ARRAY_LENGTH(illegal_nop_table[0])));
        return cpu_error(cpu,  qe6502_err_illegal_instr );
    }

    cpu->pointer.u8_0 = illegal_nop_table[i1][i2][0];


    if (cpu->pointer.u8_0 == 0 ||
        cpu->pointer.u8_0 != illegal_nop_table[i1][i2][1])
    {
        qe_log_error("Unexpected values, opcode: %u bytes: %u, cycles: %u",
               (unsigned)cpu->opcode, (unsigned)cpu->pointer.u8_0, (unsigned)illegal_nop_table[i1][i2][1]);
        return cpu_error(cpu,  qe6502_err_illegal_instr );
    }

    cpu->pointer.u8_0--; //
    return jump_to(cpu, cmos_instr_ILLEGAL_NOP_loop);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ILLEGAL_NOP_loop( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (cpu->pointer.u8_0 == 0)
    {
        return jump_to(cpu, cmos_fetch_opcode);
    }
    cpu->pointer.u8_0--;
    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to(cmos_instr_ILLEGAL_NOP_loop);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ILLEGAL_NOP_st_xXf( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (qe6502_model_impl(cpu) == qe6502_st)
    {
        if (0x1f == (cpu->opcode & 0x1f)) // $1f,$3f,$5f...$ff
        {
            request_read_dummy(cpu, (qe_word16_t){.u16=cpu->PC.u16-1},  OFFSETOF(data));
            return resume_to_dummy_read(cpu, cmos_fetch_opcode );
        }
        else if (0x0f == (cpu->opcode & 0x0f)) // $0f,$2f,$4f,...$ef
        {
            return jump_to( cpu, cmos_fetch_opcode );
        }
    }
    qe_log_error("Synertek only opcode: %u", (unsigned)(cpu->opcode));
    return cpu_error(cpu, qe6502_err_logic_error);
}

INSTR_FW_DECL(cmos_instr_0x5C_0xDC_0xFC_2);
INSTR_FW_DECL(cmos_instr_0x5C_0xDC_0xFC_3);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_0x5C_0xDC_0xFC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to_dummy_read(cpu, cmos_instr_0x5C_0xDC_0xFC_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_0x5C_0xDC_0xFC_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_0x5C_0xDC_0xFC_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_0x5C_0xDC_0xFC_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      ORA
 *
 *  Description: Logical Inclusive OR
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x01(  1)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0x05(  5)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0x09(  9)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0x0D( 13)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0x11( 17)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0x15( 21)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0x19( 25)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0x1D( 29)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ORA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A |= cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    return jump_to(cpu,  cmos_fetch_opcode);
}

/********************************************************
 *                      PHA
 *
 *  Description: Push Accumulator
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x48( 72)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PHA_2);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PHA_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHA_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(A));
    cpu->S--;
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      PHP
 *
 *  Description: Push Processor Status
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x08(  8)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PHP_2);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PHP_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHP_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data = cpu->P | qe6502_flag_B;
    request_stack_write(cpu, cpu->S, OFFSETOF(data));
    cpu->S--;
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      PLA
 *
 *  Description: Pull Accumulator
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x68(104)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PLA_2);
INSTR_FW_DECL(cmos_instr_PLA_3);
INSTR_FW_DECL(cmos_instr_PLA_4);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLA_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLA_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLA_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLA_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(A));
    return resume_to(cmos_instr_PLA_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLA_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->A) << qe6502_flagpos_Z) |
                        (cpu->A & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      PLP
 *
 *  Description: Pull Processor Status
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x28( 40)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PLP_2);
INSTR_FW_DECL(cmos_instr_PLP_3);
INSTR_FW_DECL(cmos_instr_PLP_4);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLP_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLP_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLP_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLP_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(P));
    return resume_to(cmos_instr_PLP_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLP_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_UN;
    cpu->P &= QE_U8(~qe6502_flag_B);

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      ROL
 *
 *  Description: Rotate Left
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x26( 38)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x2A( 42)  Bytes: 1,  Mode: Accumulator
 *          See also:  cmos_pre_rw_accumulator
 *
 *  Opcode: 0x2E( 46)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0x36( 54)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x3E( 62)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ROL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data >> 7;
    cpu->data = QE_U8(cpu->data << 1);
    cpu->data |= (qe6502_flag_C & cpu->P);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->data?0:qe6502_flag_Z)|
                            (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ROL_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A >> 7;
    cpu->A = QE_U8(cpu->A << 1);
    cpu->A |= (qe6502_flag_C & cpu->P);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      ROR
 *
 *  Description: Rotate Right
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x66(102)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x6A(106)  Bytes: 1,  Mode: Accumulator
 *          See also:  cmos_pre_rw_accumulator
 *
 *  Opcode: 0x6E(110)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 *  Opcode: 0x76(118)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x7E(126)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ROR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data & 1;
    cpu->data >>= 1;
    cpu->data |= QE_U8(cpu->P << 7);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->data?0:qe6502_flag_Z)|
                            (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_ROR_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A & 1;
    cpu->A >>= 1;
    cpu->A |= QE_U8(cpu->P << 7);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      RTI
 *
 *  Description: Return from Interrupt
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x40( 64)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_RTI_2);
INSTR_FW_DECL(cmos_instr_RTI_3);
INSTR_FW_DECL(cmos_instr_RTI_4);
INSTR_FW_DECL(cmos_instr_RTI_5);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_RTI_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTI_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_RTI_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTI_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(P));
    return resume_to(cmos_instr_RTI_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTI_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_UN;
    cpu->P &= QE_U8(~qe6502_flag_B);
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_0));
    return resume_to(cmos_instr_RTI_5);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTI_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_1));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      RTS
 *
 *  Description: Return from Subroutine
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x60( 96)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/


INSTR_FW_DECL(cmos_instr_RTS_2);
INSTR_FW_DECL(cmos_instr_RTS_3);
INSTR_FW_DECL(cmos_instr_RTS_4);
INSTR_FW_DECL(cmos_instr_RTS_5);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_RTS_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTS_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_RTS_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTS_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_0));
    return resume_to(cmos_instr_RTS_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTS_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_1));
    return resume_to(cmos_instr_RTS_5);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RTS_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      SBC
 *
 *  Description: Subtract with Carry
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xE1(225)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_r_indexed_x
 *
 *  Opcode: 0xE5(229)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_r_zeropage
 *
 *  Opcode: 0xE9(233)  Bytes: 2,  Mode: Immediate
 *          See also:  cmos_pre_r_immediate
 *
 *  Opcode: 0xED(237)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_r_absolute
 *
 *  Opcode: 0xF1(241)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_r_indexed_y
 *
 *  Opcode: 0xF5(245)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_r_zeropage_x
 *
 *  Opcode: 0xF9(249)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_r_absolute_y
 *
 *  Opcode: 0xFD(253)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SBC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, cmos_instr_SBC_impl_bin);
    }
    else
    {
        request_read(cpu, cpu->address, OFFSETOF(pointer.u8_0));
        return resume_to(cmos_instr_SBC_impl_dec);
    }
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SBC_Immediate( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, cmos_instr_SBC_impl_bin);
    }
    else
    {
        request_read(cpu, (qe_word16_t){.u16=0x0000}, OFFSETOF(pointer.u8_0));
        return resume_to(cmos_instr_SBC_impl_dec);
    }
}

/********************************************************
 *                      SEC
 *
 *  Description: Set Carry Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x38( 56)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SEC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_C;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      SED
 *
 *  Description: Set Decimal Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xF8(248)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SED( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_D;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      SEI
 *
 *  Description: Set Interrupt Disable
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x78(120)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_SEI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_I;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      STA
 *
 *  Description: Store Accumulator
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x81(129)  Bytes: 2,  Mode: Indexed_X
 *          See also:  cmos_pre_w_indexed_x
 *
 *  Opcode: 0x85(133)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_w_zeropage
 *
 *  Opcode: 0x8D(141)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_w_absolute
 *
 *  Opcode: 0x91(145)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  cmos_pre_w_indexed_y
 *
 *  Opcode: 0x95(149)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_w_zeropage_x
 *
 *  Opcode: 0x99(153)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  cmos_pre_w_absolute_y
 *
 *  Opcode: 0x9D(157)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_w_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_STA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(A));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      STX
 *
 *  Description: Store X Register
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x86(134)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_w_zeropage
 *
 *  Opcode: 0x8E(142)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_w_absolute
 *
 *  Opcode: 0x96(150)  Bytes: 2,  Mode: ZeroPage_Y
 *          See also:  cmos_pre_w_zeropage_y
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_STX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(X));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      STY
 *
 *  Description: Store Y Register
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x84(132)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_w_zeropage
 *
 *  Opcode: 0x8C(140)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_w_absolute
 *
 *  Opcode: 0x94(148)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_w_zeropage_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_STY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(Y));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      TAXcmos_pre_w_zeropage_2
 *
 *  Description: Transfer Accumulator to X
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xAA(170)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TAX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->A;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      TAY
 *
 *  Description: Transfer Accumulator to Y
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xA8(168)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TAY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y = cpu->A;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      TSX
 *
 *  Description: Transfer Stack Pointer to X
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xBA(186)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TSX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->S;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->S?0:qe6502_flag_Z)|
                            (cpu->S & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}
/********************************************************
 *                      TXA
 *
 *  Description: Transfer X to Accumulator
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x8A(138)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TXA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->X;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->X?0:qe6502_flag_Z)|
                            (cpu->X & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      TXS
 *
 *  Description: Transfer X to Stack Pointer
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x9A(154)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TXS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S = cpu->X;

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      TYA
 *
 *  Description: Transfer Y to Accumulator
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x98(152)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TYA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->Y;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->Y?0:qe6502_flag_Z)|
                            (cpu->Y & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_fetch_opcode);
}


/********************************************************
 *                      BRA
 *
 *  Description: Branch if Always
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x80(128)  Bytes: 2,  Mode: Relative
 *          See also:  cmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_BRA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    return resume_to(cmos_do_relative_branch);
}

/********************************************************
 *                      PHX
 *
 *  Description: Push Register X
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0xDA(218)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PHX_2);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PHX_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHX_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(X));
    cpu->S--;
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      PHY
 *
 *  Description: Push Register Y
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x5A( 90)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PHY_2);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PHY_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PHY_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(Y));
    cpu->S--;
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      PLX
 *
 *  Description: Pull Register X
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0xFA(250)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PLX_2);
INSTR_FW_DECL(cmos_instr_PLX_3);
INSTR_FW_DECL(cmos_instr_PLX_4);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLX_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLX_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLX_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLX_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(X));
    return resume_to(cmos_instr_PLX_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLX_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->X) << qe6502_flagpos_Z) |
                        (cpu->X & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      PLY
 *
 *  Description: Pull Register Y
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x7A(122)  Bytes: 1,  Mode: Implied
 *          See also:  cmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(cmos_instr_PLY_2);
INSTR_FW_DECL(cmos_instr_PLY_3);
INSTR_FW_DECL(cmos_instr_PLY_4);

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLY_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLY_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cmos_instr_PLY_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLY_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(Y));
    return resume_to(cmos_instr_PLY_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_PLY_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->Y) << qe6502_flagpos_Z) |
                        (cpu->Y & qe6502_flag_N)));

    return jump_to(cpu, cmos_fetch_opcode);
}

/********************************************************
 *                      STZ
 *
 *  Description: Store Zero
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x64(100)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_w_zeropage
 *
 *  Opcode: 0x9C(145)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_w_absolute
 *
 *  Opcode: 0x74(116)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  cmos_pre_w_zeropage_x
 *
 *  Opcode: 0x9E(158)  Bytes: 3,  Mode: Absolute_X
 *          See also:  cmos_pre_w_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_STZ( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data = 0;
    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      TRB
 *
 *  Description: Test and Reset Bits
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x14( 20)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x1C( 28)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TRB( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z,
                        ((cpu->A & cpu->data) == 0)? qe6502_flag_Z : 0);
    cpu->data &= ~cpu->A;

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

/********************************************************
 *                      TSB
 *
 *  Description: Test and Set Bits
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x04( 04)  Bytes: 2,  Mode: ZeroPage
 *          See also:  cmos_pre_rw_zeropage
 *
 *  Opcode: 0x0C( 12)  Bytes: 3,  Mode: Absolute
 *          See also:  cmos_pre_rw_absolute
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_TSB( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z,
                        ((cpu->A & cpu->data) == 0)? qe6502_flag_Z : 0);
    cpu->data |= cpu->A;

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}


/********************************************************
 *                      RMB/SMB
 *
 *  Description: RMB SMB - Reset(RMB) or Set(SMB) Memory Bit
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: RMB 0..7  (0x07,0x17,...,0x67,0x77)
 *          SMB 0..7  (0x87,0x97,...,0xE7,0xF7)
 *          Bytes: 2,  Mode: ZeroPage
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_instr_RMB_SMB( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (qe6502_model_impl(cpu) == qe6502_st)
    {
        return jump_to(cpu, cmos_fetch_opcode);
    }
    uint8_t bit = cpu->opcode >> 4;
    if (bit < 8)
    {
        // RMB
        cpu->data &= QE_U8(~(1 << bit));
    }
    else
    {
        // SMB
        bit -= 8;
        cpu->data |= QE_U8(1 << bit);
    }
    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_fetch_opcode);
}

/////////////////////////////////////////////////////////////////////////////////////

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: Jmp
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_jmp_absolute_2 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_jmp_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    cpu->PC.u16++;

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_absolute_2 );
INSTR_FW_DECL( cmos_pre_r_absolute_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_r_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    cpu->PC.u16++;

    return resume_to( cmos_pre_r_absolute_3 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_rw_absolute_2 );
INSTR_FW_DECL( cmos_pre_rw_absolute_3 );
INSTR_FW_DECL( cmos_pre_rw_absolute_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_rw_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    cpu->PC.u16++;

    return resume_to( cmos_pre_rw_absolute_3 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cmos_pre_rw_absolute_4 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_read( cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_absolute_2 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_w_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    cpu->PC.u16++;

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_X
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_absolute_x_2 );
INSTR_FW_DECL( cmos_pre_r_absolute_x_3 );
INSTR_FW_DECL( cmos_pre_r_absolute_x_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_r_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_r_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (cpu->pagecross_overflow)
    {
        request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
        cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_pre_r_absolute_x_4 );
    }
    else
    {
        request_read(cpu, cpu->address, OFFSETOF(data));
        cpu->PC.u16++;
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_X
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_rw_absolute_x_2 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_3 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_4 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;

    return resume_to(cmos_pre_rw_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_rw_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));

    cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
    cpu->PC.u16++;

    return resume_to_dummy_read(cpu, cmos_pre_rw_absolute_x_4 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cmos_pre_rw_absolute_x_5 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cpu->instr );
}

INSTR_FW_DECL( cmos_pre_rw_absolute_x_short_2 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_short_3 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_short_4 );
INSTR_FW_DECL( cmos_pre_rw_absolute_x_short_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_short( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;

    return resume_to(cmos_pre_rw_absolute_x_short_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_short_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));
    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_rw_absolute_x_short_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_short_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (cpu->pagecross_overflow)
    {
        request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
        cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_pre_rw_absolute_x_short_4 );
    }
    request_read(cpu, cpu->address, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to( cmos_pre_rw_absolute_x_short_5 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_short_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cmos_pre_rw_absolute_x_short_5 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_absolute_x_short_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_X
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_absolute_x_2 );
INSTR_FW_DECL( cmos_pre_w_absolute_x_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_w_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_w_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));

    cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
    cpu->PC.u16++;

    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_absolute_y_2 );
INSTR_FW_DECL( cmos_pre_r_absolute_y_3 );
INSTR_FW_DECL( cmos_pre_r_absolute_y_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;

    return resume_to(cmos_pre_r_absolute_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_r_absolute_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{

    if (cpu->pagecross_overflow)
    {
        request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
        cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_pre_r_absolute_y_4 );
    }
    else
    {
        request_read(cpu, cpu->address, OFFSETOF(data));
        cpu->PC.u16++;
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_absolute_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_Y
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_absolute_y_2 );
INSTR_FW_DECL( cmos_pre_w_absolute_y_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_w_absolute_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_w_absolute_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_absolute_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));

    cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
    cpu->PC.u16++;

    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Immediate
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_immediate( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;

    return resume_to(cpu->instr);
}

/********************************************************
 *
 *  Mode:  Indexed_X
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_indexed_x_2 );
INSTR_FW_DECL( cmos_pre_r_indexed_x_3 );
INSTR_FW_DECL( cmos_pre_r_indexed_x_4 );
INSTR_FW_DECL( cmos_pre_r_indexed_x_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));
    cpu->PC.u16++;
    cpu->pointer.u8_1 = 0;

    return resume_to(cmos_pre_r_indexed_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->pointer,  OFFSETOF(address.u8_0));

    cpu->pointer.u8_0 += cpu->X;

    return resume_to_dummy_read(cpu, cmos_pre_r_indexed_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));

    cpu->pointer.u8_0++;

    return resume_to(cmos_pre_r_indexed_x_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));

    return resume_to( cmos_pre_r_indexed_x_5 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_x_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indexed_X
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_indexed_x_2 );
INSTR_FW_DECL( cmos_pre_w_indexed_x_3 );
INSTR_FW_DECL( cmos_pre_w_indexed_x_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_w_indexed_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_1 = 0;
    request_read_dummy(cpu, cpu->pointer,  OFFSETOF(address.u8_0));

    cpu->pointer.u8_0 += cpu->X;

    return resume_to_dummy_read(cpu, cmos_pre_w_indexed_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));

    cpu->pointer.u8_0++;

    return resume_to( cmos_pre_w_indexed_x_4 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indexed_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_indexed_y_2 );
INSTR_FW_DECL( cmos_pre_r_indexed_y_3 );
INSTR_FW_DECL( cmos_pre_r_indexed_y_4 );
INSTR_FW_DECL( cmos_pre_r_indexed_y_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));

    return resume_to(cmos_pre_r_indexed_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_1 = 0;
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));

    return resume_to(cmos_pre_r_indexed_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_0++;

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));

    return resume_to(cmos_pre_r_indexed_y_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (cpu->pagecross_overflow)
    {
        cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
        request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_pre_r_indexed_y_5 );
    }
    else
    {
        request_read(cpu, cpu->address, OFFSETOF(data));
        cpu->PC.u16++;
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indexed_y_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indexed_Y
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_indexed_y_2 );
INSTR_FW_DECL( cmos_pre_w_indexed_y_3 );
INSTR_FW_DECL( cmos_pre_w_indexed_y_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));

    return resume_to(cmos_pre_w_indexed_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_1 = 0;
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));
    cpu->pointer.u8_0++;
    return resume_to(cmos_pre_w_indexed_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));

    cpu->address.u8_1 = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_1);

    return resume_to(cmos_pre_w_indexed_y_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indexed_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));

    cpu->address.u8_1 = QE_U8(cpu->address.u8_1 + cpu->pagecross_overflow);
    cpu->PC.u16++;

    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indirect
 *
 *  Class: Jmp
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_jmp_indirect_2 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_3 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_4 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_jmp_indirect_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_1));
    cpu->PC.u16++;

    return resume_to(cmos_pre_jmp_indirect_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_0));

    // An original 6502 has does not correctly fetch the target
    // address if the indirect vector falls on a page boundary
    // (e.g. $xxFF where xx is any value from $00 to $FF).
    // In this case fetches the LSB from $xxFF as expected
    // but takes the MSB from $xx00.
    // This is fixed in some later chips like the 65SC02
    cpu->pointer.u8_0++;
    cpu->pagecross_overflow = (cpu->pointer.u8_0 == 0);

    return resume_to(cmos_pre_jmp_indirect_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_1));
    cpu->pointer.u8_1 = QE_U8(cpu->pointer.u8_1 + cpu->pagecross_overflow);
    return resume_to(cmos_pre_jmp_indirect_5);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_1));
    return resume_to(cmos_instr_JMP);
}

/********************************************************
 *
 *  Mode:  Indirect_X
 *
 *  Class: Jmp
 *
 ********************************************************/


INSTR_FW_DECL( cmos_pre_jmp_indirect_x_2 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_x_3 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_x_4 );
INSTR_FW_DECL( cmos_pre_jmp_indirect_x_5 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));
    cpu->PC.u16++;

    return resume_to(cmos_pre_jmp_indirect_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_1));
    cpu->PC.u16++;

    return resume_to(cmos_pre_jmp_indirect_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, (qe_word16_t){.u16 = cpu->PC.u16 - 2}, OFFSETOF(address.u8_1));
    return resume_to_dummy_read(cpu, cmos_pre_jmp_indirect_x_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u16 += cpu->X;
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_0));
    cpu->pointer.u16++;

    return resume_to(cmos_pre_jmp_indirect_x_5);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_jmp_indirect_x_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_1));
    return resume_to(cmos_instr_JMP);
}

/********************************************************
 *
 *  Mode:  ZeroPage
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_zeropage_2 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;
    return resume_to(cmos_pre_r_zeropage_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_rw_zeropage_2 );
INSTR_FW_DECL( cmos_pre_rw_zeropage_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_rw_zeropage_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to(cmos_pre_rw_zeropage_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    return resume_to_dummy_read(cpu, cpu->instr);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_RMB_SMB( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (qe6502_model_impl(cpu) == qe6502_st)
    {
        if (0x17 == (cpu->opcode & 0x17)) // $17,$37,$57...$f7
        {
            // RMB
            return jump_to(cpu, cmos_pre_r_zeropage_x);
        }
        else if (0x07 == (cpu->opcode & 0x07)) // $07,$27,$47...$e7
        {
            return jump_to(cpu, cmos_pre_r_zeropage);
        }
        else
        {
            qe_log_error("Illegal Synertek BIT opcode: %u", (unsigned)cpu->opcode);
            return cpu_error(cpu, qe6502_err_logic_error);
        }
    }
    return jump_to(cpu, cmos_pre_rw_zeropage);
}



/********************************************************
 *
 *  Mode:  ZeroPage
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage_X
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_zeropage_x_2 );
INSTR_FW_DECL( cmos_pre_r_zeropage_x_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_r_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_0 += cpu->X;

    return resume_to_dummy_read(cpu, cmos_pre_r_zeropage_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage_X
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_rw_zeropage_x_2 );
INSTR_FW_DECL( cmos_pre_rw_zeropage_x_3 );
INSTR_FW_DECL( cmos_pre_rw_zeropage_x_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_rw_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_0 += cpu->X;

    return resume_to_dummy_read(cpu, cmos_pre_rw_zeropage_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    return resume_to(cmos_pre_rw_zeropage_x_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_zeropage_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    return resume_to_dummy_read(cpu, cpu->instr);
}

/********************************************************
 *
 *  Mode:  ZeroPage_X
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_zeropage_x_2 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_w_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    cpu->address.u8_0 += cpu->X;

    return resume_to(cpu->instr);
}

/********************************************************
 *
 *  Mode:  ZeroPage_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_zeropage_y_2 );
INSTR_FW_DECL( cmos_pre_r_zeropage_y_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;

    return resume_to(cmos_pre_r_zeropage_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_1 = 0;
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_0 += cpu->Y;

    return resume_to_dummy_read(cpu, cmos_pre_r_zeropage_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_zeropage_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage_Y
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_zeropage_y_2 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_zeropage_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));

    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_w_zeropage_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_zeropage_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_0 += cpu->Y;

    return resume_to_dummy_read(cpu, cpu->instr);
}

/********************************************************
 *
 *  Mode:  Indirect,ZP (CMOS 65C02)
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_r_indirect_zp_2 );
INSTR_FW_DECL( cmos_pre_r_indirect_zp_3 );
INSTR_FW_DECL( cmos_pre_r_indirect_zp_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indirect_zp( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));

    cpu->PC.u16++;
    cpu->pointer.u8_1 = 0;

    return resume_to(cmos_pre_r_indirect_zp_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indirect_zp_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));
    cpu->pointer.u8_0++;
    return resume_to(cmos_pre_r_indirect_zp_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indirect_zp_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));
    return resume_to(cmos_pre_r_indirect_zp_4);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_r_indirect_zp_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address,  OFFSETOF(data));
    return resume_to( cpu->instr );
}


/********************************************************
 *
 *  Mode:  Indirect,ZP (CMOS 65C02)
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_w_indirect_zp_2 );
INSTR_FW_DECL( cmos_pre_w_indirect_zp_3 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indirect_zp( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_0));

    cpu->PC.u16++;
    cpu->pointer.u8_1 = 0;

    return resume_to(cmos_pre_w_indirect_zp_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indirect_zp_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_0));
    cpu->pointer.u8_0++;
    return resume_to(cmos_pre_w_indirect_zp_3);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_w_indirect_zp_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_1));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage_Relative (CMOS 65C02)
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

/********************************************************
 *                      BBR/BBS
 *
 *  Description: Branch on Bit Reset(BBR) or Set(BBS)
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: BBR 0..7  (0x0F,0x1F,...,0x6F,0x7F)
 *          BBS 0..7  (0x8F,0x9F,...,0xEF,0xFF)
 *
 *          Bytes: 3,  Mode: ZeroPage_Relative (CMOS 65C02)
 *
 *
 ********************************************************/

INSTR_FW_DECL( cmos_pre_rw_bbr_zeropage_relative_2 );
INSTR_FW_DECL( cmos_pre_rw_bbr_zeropage_relative_3 );
INSTR_FW_DECL( cmos_pre_rw_bbr_zeropage_relative_4 );

INSTR_FW_DECL(cmos_pre_rw_bbr_bbs_zeropage_relative_6);
INSTR_FW_DECL(cmos_pre_rw_bbr_bbs_zeropage_relative_7);

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_zeropage_relative( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (qe6502_model_impl(cpu) == qe6502_st)
    {
        return jump_to(cpu, cmos_pre_w_absolute);
    }
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_rw_bbr_zeropage_relative_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_zeropage_relative_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address,  OFFSETOF(data));
    return resume_to( cmos_pre_rw_bbr_zeropage_relative_3 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_zeropage_relative_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address,  OFFSETOF(data));
    return resume_to( cmos_pre_rw_bbr_zeropage_relative_4 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_zeropage_relative_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t bit = cpu->opcode >> 4;
    // BBR
    if (cpu->data & (1 << bit))
    {
        request_read_dummy(cpu, cpu->PC, OFFSETOF(address.u8_0));
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }

    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    return resume_to(cmos_pre_rw_bbr_bbs_zeropage_relative_6);
}

INSTR_FW_DECL( cmos_pre_rw_bbs_zeropage_relative_2 );
INSTR_FW_DECL( cmos_pre_rw_bbs_zeropage_relative_3 );
INSTR_FW_DECL( cmos_pre_rw_bbs_zeropage_relative_4 );

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbs_zeropage_relative( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (qe6502_model_impl(cpu) == qe6502_st)
    {
        return jump_to(cpu, cmos_pre_w_absolute);
    }
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    cpu->address.u8_1 = 0;

    return resume_to(cmos_pre_rw_bbs_zeropage_relative_2);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbs_zeropage_relative_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address,  OFFSETOF(data));
    return resume_to( cmos_pre_rw_bbs_zeropage_relative_3 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbs_zeropage_relative_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address,  OFFSETOF(data));
    return resume_to( cmos_pre_rw_bbs_zeropage_relative_4 );
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbs_zeropage_relative_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t bit = cpu->opcode >> 4;
    // BBS
    bit -= 8;
    if (!(cpu->data & (1 << bit)))
    {
        request_read_dummy(cpu, cpu->PC, OFFSETOF(address.u8_0));
        cpu->PC.u16++;
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_0));
    cpu->PC.u16++;
    return resume_to(cmos_pre_rw_bbr_bbs_zeropage_relative_6);
}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_bbs_zeropage_relative_6( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->pointer.u16 = QE_U16(cpu->PC.u16 + cpu->address.i8_0);
    if (cpu->pointer.u8_1 == cpu->PC.u8_1)
    {
        // no page crossing
        cpu->PC = cpu->pointer;
        return resume_to_dummy_read(cpu, cmos_fetch_opcode);
    }
    return resume_to_dummy_read(cpu, cmos_pre_rw_bbr_bbs_zeropage_relative_7);

}

INSTR_RETTYPE qe6502_cycle_t
cmos_pre_rw_bbr_bbs_zeropage_relative_7( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC = cpu->pointer;
    return resume_to(cmos_fetch_opcode);
}


/********************************************************
 *
 *
 *  NMI/IRQ Interrupts Routineс
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t  //
cmos_irq( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_0++;
    switch(cpu->address.u8_0)
    {
    case 1:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_1));
        cpu->S--;
        break;
    case 2:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_0));
        cpu->S--;
        break;
    case 3:
        request_stack_write(cpu, cpu->S, OFFSETOF(P));
        cpu->S--;
        break;
    case 4:
        cpu->P |= qe6502_flag_I;
        request_read(cpu, (qe_word16_t){.u16=0xfffe}, OFFSETOF(PC.u8_0));
        break;
    case 5:
        request_read(cpu, (qe_word16_t){.u16=0xffff}, OFFSETOF(PC.u8_1));
        return resume_to(cmos_fetch_opcode);
    default:
        qe_log_error("IRQ interrupt unexpected state");
        return cpu_error(cpu,  qe6502_err_corrupt_state);
    }
    return resume_to(cmos_irq);
}

INSTR_RETTYPE qe6502_cycle_t  //
cmos_nmi( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_0++;
    switch(cpu->address.u8_0)
    {
    case 1:
        cpu->istate &= QE_U8(~qe6502_nmi_pin_chg);
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_1));
        cpu->S--;
        break;
    case 2:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_0));
        cpu->S--;
        break;
    case 3:
        request_stack_write(cpu, cpu->S, OFFSETOF(P));
        cpu->S--;
        break;
    case 4:
        cpu->P |= qe6502_flag_I;
        request_read(cpu, (qe_word16_t){.u16=0xfffa}, OFFSETOF(PC.u8_0));
        break;
    case 5:
        request_read(cpu, (qe_word16_t){.u16=0xfffb}, OFFSETOF(PC.u8_1));
        return resume_to(cmos_fetch_opcode);
    default:
        qe_log_error("NMI interrupt Error, unexpected!");
        return cpu_error(cpu,  qe6502_err_corrupt_state);
    }
    return resume_to(cmos_nmi);
}
#else
    typedef int qe_cmos_empty_translation_unit_dummy;
#endif // QE6502_ENABLE_CMOS_65C02
