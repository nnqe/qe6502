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

#if (QE6502_ENABLE_NMOS_6502 == 1)

#include "nmos_opcodes.h"

/********************************************************
 *
 *  Start new instruction
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_opcode_dispatcher( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC.u16++;
    cpu->instr = nmos_model.opcodes[cpu->opcode].instr;
    return jump_to(cpu,  nmos_model.opcodes[cpu->opcode].entry );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_fetch_opcode( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(opcode));

    cpu->cmd.flags |= qe6502_wait_opcode;

    return resume_to( nmos_opcode_dispatcher );
}

qe6502_cycle_t mos_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    return nmos_fetch_opcode(cpu);
}

qe6502_cycle_t nes_fetch_opcode_bridge( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    return nmos_fetch_opcode(cpu);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ILLEGAL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    qe_log("qe6502", "Illegal instruction opcode: %u, (%02X)", (unsigned)cpu->opcode, (unsigned)cpu->opcode);
    return cpu_error(cpu,  qe6502_err_illegal_instr );
}

////////////////////////////////////////////////////////////////////////

INSTR_FW_DECL(nmos_do_relative_branch_2);

INSTR_RETTYPE qe6502_cycle_t
nmos_do_relative_branch( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    qe_word_t pc;
    pc.u16 = QE_U16(cpu->PC.u16 + cpu->address.i8_lsb);
    cpu->PC.u8_lsb = pc.u8_lsb;
    if (pc.u8_msb == cpu->PC.u8_msb)
    {
        // no page crossing
        return resume_to(nmos_fetch_opcode);
    }
    cpu->address = pc;
    return resume_to_dummy_read(cpu, nmos_do_relative_branch_2);

}

INSTR_RETTYPE qe6502_cycle_t
nmos_do_relative_branch_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC = cpu->address;
    return resume_to(nmos_fetch_opcode);
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
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0x65(101)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0x69(105)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0x6D(109)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0x71(113)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0x75(117)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0x79(121)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0x7D(125)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ADC_impl_bin( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t u = cpu->data;
    qe_word_t r16 = (qe_word_t){.u16 = cpu->A};

    uint8_t carry = ((cpu->P & qe6502_flag_C ) >> qe6502_flagpos_C);
    r16.u16 += QE_U16(u + carry);
    uint8_t c = r16.u16 > 0xff;

    uint8_t flags = QE_U8(
                    (c) |  // carry
                    ((r16.u8_lsb == 0) << qe6502_flagpos_Z) | // zero
                    (r16.u8_lsb & qe6502_flag_N) |  // negative
                    (((((~(cpu->A ^ u)) & (cpu->A ^ r16.u8_lsb)) & qe6502_flag_N) != 0) << qe6502_flagpos_V)); // overflow

    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    cpu->A = r16.u8_lsb;

    return jump_to(cpu, nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ADC_impl_dec( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t data = cpu->data;
    uint8_t flags = 0;

    uint8_t carry = ((cpu->P & qe6502_flag_C ) >> qe6502_flagpos_C);
    uint8_t bin_result = QE_U8(cpu->A + data + carry);
    flags |= QE_U8(((bin_result == 0) << qe6502_flagpos_Z));

    uint8_t low = QE_U8((cpu->A & 0x0f) + (data & 0x0f) + carry);
    uint8_t high = (cpu->A >> 4) + (data >> 4);
    if (low > 9)
    {
        low -= 10;
        high += 1;
    }

    uint8_t result = QE_U8((high << 4) | (low & 0x0F));
    flags |= (result & qe6502_flag_N);
    if ((~(cpu->A ^ data) & (cpu->A ^ result)) & qe6502_flag_N)
    {
        flags |= qe6502_flag_V;
    }

    if (high > 9)
    {
        result -= (10 * 16);
        flags |= 1; // carry
    }

    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    cpu->A = result;

    return jump_to(cpu, nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SBC_impl_bin( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data ^= 0xff;
    return jump_to(cpu, nmos_instr_ADC_impl_bin);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SBC_impl_dec( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t data = cpu->data;
    uint8_t flags = qe6502_flag_C;

    uint8_t carry = ((cpu->P & qe6502_flag_C ) >> qe6502_flagpos_C);

    uint8_t bin_result = QE_U8(cpu->A + (data^0xff) + carry);
    flags |= QE_U8((bin_result == 0) << qe6502_flagpos_Z);

    int8_t carryInv = !carry;
    int8_t low = QE_S8((int8_t)(cpu->A & 0x0F) - (int8_t)(data & 0x0F) - (int8_t)carryInv);
    int8_t high = (int8_t)(cpu->A >> 4) - (int8_t)(data >> 4);

    if (low < 0)
    {
        low += 10;
        high -= 1;
    }
    uint8_t result = QE_U8((high << 4) | (low & 0x0F));
    flags |= (result & qe6502_flag_N);
    if ( ((cpu->A ^ data) & (cpu->A ^ result)) & qe6502_flag_N )
    {
        flags |= qe6502_flag_V;
    }

    if (high < 0)
    {
        result += (10 * 16);
        flags &= ~qe6502_flag_C; // clear carry
    }

    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N | qe6502_flag_V, flags);
    cpu->A = result;

    return jump_to(cpu, nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ADC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, nmos_instr_ADC_impl_bin);
    }
    else
    {
        if (cpu->model == qe6502_nes)
        {
            return jump_to(cpu, nmos_instr_ADC_impl_bin);
        }
        else
        {
            return jump_to(cpu, nmos_instr_ADC_impl_dec);
        }
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
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0x25( 37)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0x29( 41)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0x2D( 45)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0x31( 49)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0x35( 53)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0x39( 57)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0x3D( 61)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_AND( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A &= cpu->data;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                        (cpu->A?0:qe6502_flag_Z)|
                        (cpu->A & qe6502_flag_N));
    // now
    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      ASL
 *
 *  Description: Arithmetic Shift Left
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x06(  6)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0x0A( 10)  Bytes: 1,  Mode: Accumulator
 *          See also:  nmos_pre_rw_accumulator
 *
 *  Opcode: 0x0E( 14)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0x16( 22)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x1E( 30)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ASL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data >> 7;
    cpu->data <<= 1;
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                        carry|
                        (cpu->data?0:qe6502_flag_Z)|
                        (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ASL_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A >> 7;
    cpu->A <<= 1;
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                        carry|
                        (cpu->A?0:qe6502_flag_Z)|
                        (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      BCC
 *
 *  Description: Branch if Carry Clear
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x90(144)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BCC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_C)
    {
        // carry not clear
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BCS
 *
 *  Description: Branch if Carry Set
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xB0(176)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BCS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_C))
    {
        // carry not set
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BEQ
 *
 *  Description: Branch if Equal
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xF0(240)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BEQ( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_Z))
    {
        // zero is clear
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BIT
 *
 *  Description: Bit Test
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x24( 36)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0x2C( 44)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BIT( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{

    update_flags(cpu, qe6502_flag_Z | qe6502_flag_V | qe6502_flag_N,
                            QE_U8(
                                    ((!(cpu->A & cpu->data)) << qe6502_flagpos_Z) |
                                    (cpu->data & (qe6502_flag_V | qe6502_flag_N))));

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      BMI
 *
 *  Description: Branch if Minus
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x30( 48)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BMI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_N))
    {
        // negative is clear
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BNE
 *
 *  Description: Branch if Not Equal
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0xD0(208)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BNE( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_Z)
    {
        // zero is set (equal)
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BPL
 *
 *  Description: Branch if Positive
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x10( 16)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BPL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_N)
    {
        // zero is set (equal)
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BRK
 *
 *  Description: Force Interrupt
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x00(  0)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_BRK_2);
INSTR_FW_DECL(nmos_instr_BRK_finalize);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BRK( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    // we are not clearing our tmp memory, so prepare memory and go to switch-case
    // we will use address lo
    cpu->address.u8_lsb = 0;
    return resume_to_dummy_read(cpu, nmos_instr_BRK_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BRK_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_lsb++;
    switch(cpu->address.u8_lsb)
    {
    case 1:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_msb));
        cpu->S--;
        break;
    case 2:
        request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_lsb));
        cpu->S--;
        break;
    case 3:
        cpu->data = cpu->P | qe6502_flag_B;
        request_stack_write(cpu, cpu->S, OFFSETOF(data));
        cpu->S--;
        break;
    case 4:
        request_read(cpu, (qe_word_t){.u16=0xfffe}, OFFSETOF(PC.u8_lsb));
        break;
    case 5:
        return jump_to(cpu, cpu->instr);
    default:
        qe_log("qe6502", "BRK Error, unexpected!");
        return cpu_error(cpu,  qe6502_err_logic_error );
    }
    return resume_to(nmos_instr_BRK_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BRK_finalize( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, (qe_word_t){.u16=0xffff}, OFFSETOF(PC.u8_msb));
    cpu->P |= qe6502_flag_I;
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      BVC
 *
 *  Description: Branch if Overflow Clear
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x50( 80)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BVC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (cpu->P & qe6502_flag_V)
    {
        // overflow set
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      BVS
 *
 *  Description: Branch if Overflow Set
 *
 *  Class: BranchIf
 *  Variants:
 *  Opcode: 0x70(112)  Bytes: 2,  Mode: Relative
 *          See also:  nmos_pre_bif_relative
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_BVS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    if (!(cpu->P & qe6502_flag_V))
    {
        // overflow clear
        return resume_to_dummy_read(cpu, nmos_fetch_opcode);
    }
    return resume_to(nmos_do_relative_branch);
}

/********************************************************
 *                      CLC
 *
 *  Description: Clear Carry Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x18( 24)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CLC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= ( ~qe6502_flag_C );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CLD
 *
 *  Description: Clear Decimal Mode
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xD8(216)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CLD( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= ( ~qe6502_flag_D );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CLI
 *
 *  Description: Clear Interrupt Disable
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x58( 88)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CLI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= ( ~qe6502_flag_I );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CLV
 *
 *  Description: Clear Overflow Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xB8(184)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CLV( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P &= ( ~qe6502_flag_V );
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CMP
 *
 *  Description: Compare
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xC1(193)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0xC5(197)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xC9(201)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xCD(205)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0xD1(209)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0xD5(213)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0xD9(217)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0xDD(221)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CMP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                 QE_U8(
                        (cpu->A >= cpu->data) |
                        ((cpu->A == cpu->data) << qe6502_flagpos_Z) |
                        ((cpu->A - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CPX
 *
 *  Description: Compare X Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xE0(224)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xE4(228)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xEC(236)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CPX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        (cpu->X >= cpu->data) |
                            ((cpu->X == cpu->data) << qe6502_flagpos_Z) |
                            ((cpu->X - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      CPY
 *
 *  Description: Compare Y Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xC0(192)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xC4(196)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xCC(204)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_CPY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            (cpu->Y >= cpu->data) |
                            ((cpu->Y == cpu->data) << qe6502_flagpos_Z) |
                            ((cpu->Y - cpu->data) & qe6502_flag_N)));

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      DEC
 *
 *  Description: Decrement Memory
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0xC6(198)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0xCE(206)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0xD6(214)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0xDE(222)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_DEC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      DEX
 *
 *  Description: Decrement X Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xCA(202)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_DEX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      DEY
 *
 *  Description: Decrement Y Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x88(136)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_DEY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y--;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      EOR
 *
 *  Description: Exclusive OR
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x41( 65)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0x45( 69)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0x49( 73)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0x4D( 77)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0x51( 81)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0x55( 85)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0x59( 89)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0x5D( 93)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_EOR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A ^= cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      INC
 *
 *  Description: Increment Memory
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0xE6(230)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0xEE(238)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0xF6(246)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0xFE(254)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_INC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      INX
 *
 *  Description: Increment X Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xE8(232)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_INX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      INY
 *
 *  Description: Increment Y Register
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xC8(200)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_INY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y++;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      JMP
 *
 *  Description: Jump
 *
 *  Class: Jmp
 *  Variants:
 *  Opcode: 0x4C( 76)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_jmp_absolute
 *
 *  Opcode: 0x6C(108)  Bytes: 3,  Mode: Indirect
 *          See also:  nmos_pre_jmp_indirect
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JMP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC = cpu->address;
    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      JSR
 *
 *  Description: Jump to Subroutine
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x20( 32)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_sp_absolute
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_JSR_2);
INSTR_FW_DECL(nmos_instr_JSR_3);
INSTR_FW_DECL(nmos_instr_JSR_4);
INSTR_FW_DECL(nmos_instr_JSR_5);
INSTR_FW_DECL(nmos_instr_JSR_6);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    return resume_to(nmos_instr_JSR_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_JSR_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_msb));
    cpu->S--;
    return resume_to(nmos_instr_JSR_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(PC.u8_lsb));
    cpu->S--;
    return resume_to(nmos_instr_JSR_5);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    return resume_to(nmos_instr_JSR_6);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_JSR_6( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->PC = cpu->address;
    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      LDA
 *
 *  Description: Load Accumulator
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA1(161)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0xA5(165)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xA9(169)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xAD(173)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0xB1(177)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0xB5(181)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0xB9(185)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0xBD(189)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_LDA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));
    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      LDX
 *
 *  Description: Load X Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA2(162)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xA6(166)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xAE(174)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0xB6(182)  Bytes: 2,  Mode: ZeroPage_Y
 *          See also:  nmos_pre_r_zeropage_y
 *
 *  Opcode: 0xBE(190)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_LDX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->X) << qe6502_flagpos_Z) |
                            (cpu->X & qe6502_flag_N)));

    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      LDY
 *
 *  Description: Load Y Register
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xA0(160)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xA4(164)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xAC(172)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0xB4(180)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0xBC(188)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_LDY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y = cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->Y) << qe6502_flagpos_Z) |
                            (cpu->Y & qe6502_flag_N)));
    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      LSR
 *
 *  Description: Logical Shift Right
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x46( 70)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0x4A( 74)  Bytes: 1,  Mode: Accumulator
 *          See also:  nmos_pre_rw_accumulator
 *
 *  Opcode: 0x4E( 78)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0x56( 86)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x5E( 94)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_LSR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data & 1;
    cpu->data >>= 1;
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            carry |
                            ((0 == cpu->data) << qe6502_flagpos_Z) |
                            (cpu->data & qe6502_flag_N)));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_LSR_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A & 1;
    cpu->A >>= 1;
    update_flags(cpu, qe6502_flag_C | qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            carry |
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      NOP
 *
 *  Description: No Operation
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0xEA(234)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_NOP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      ORA
 *
 *  Description: Logical Inclusive OR
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0x01(  1)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0x05(  5)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0x09(  9)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0x0D( 13)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0x11( 17)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0x15( 21)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0x19( 25)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0x1D( 29)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ORA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A |= cpu->data;
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                            QE_U8(
                            ((0 == cpu->A) << qe6502_flagpos_Z) |
                            (cpu->A & qe6502_flag_N)));

    return jump_to(cpu,  nmos_fetch_opcode);
}

/********************************************************
 *                      PHA
 *
 *  Description: Push Accumulator
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x48( 72)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_PHA_2);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PHA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PHA_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PHA_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_write(cpu, cpu->S, OFFSETOF(A));
    cpu->S--;
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      PHP
 *
 *  Description: Push Processor Status
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x08(  8)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_PHP_2);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PHP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PHP_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PHP_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->data = cpu->P | qe6502_flag_B;
    request_stack_write(cpu, cpu->S, OFFSETOF(data));
    cpu->S--;
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      PLA
 *
 *  Description: Pull Accumulator
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x68(104)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_PLA_2);
INSTR_FW_DECL(nmos_instr_PLA_3);
INSTR_FW_DECL(nmos_instr_PLA_4);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PLA_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLA_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PLA_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLA_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(A));
    return resume_to(nmos_instr_PLA_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLA_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    update_flags(cpu, qe6502_flag_Z | qe6502_flag_N,
                        QE_U8(
                        ((0 == cpu->A) << qe6502_flagpos_Z) |
                        (cpu->A & qe6502_flag_N)));

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      PLP
 *
 *  Description: Pull Processor Status
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x28( 40)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_PLP_2);
INSTR_FW_DECL(nmos_instr_PLP_3);
INSTR_FW_DECL(nmos_instr_PLP_4);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLP( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PLP_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLP_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_PLP_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLP_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(P));
    return resume_to(nmos_instr_PLP_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_PLP_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_UN;
    cpu->P &= ~qe6502_flag_B;

    return jump_to(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      ROL
 *
 *  Description: Rotate Left
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x26( 38)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0x2A( 42)  Bytes: 1,  Mode: Accumulator
 *          See also:  nmos_pre_rw_accumulator
 *
 *  Opcode: 0x2E( 46)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0x36( 54)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x3E( 62)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ROL( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data >> 7;
    cpu->data <<= 1;
    cpu->data |= (qe6502_flag_C & cpu->P);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->data?0:qe6502_flag_Z)|
                            (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ROL_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A >> 7;
    cpu->A <<= 1;
    cpu->A |= (qe6502_flag_C & cpu->P);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      ROR
 *
 *  Description: Rotate Right
 *
 *  Class: ReaderWriter
 *  Variants:
 *  Opcode: 0x66(102)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_rw_zeropage
 *
 *  Opcode: 0x6A(106)  Bytes: 1,  Mode: Accumulator
 *          See also:  nmos_pre_rw_accumulator
 *
 *  Opcode: 0x6E(110)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_rw_absolute
 *
 *  Opcode: 0x76(118)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_rw_zeropage_x
 *
 *  Opcode: 0x7E(126)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_rw_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ROR( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->data & 1;
    cpu->data >>= 1;
    cpu->data |= (cpu->P << 7);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->data?0:qe6502_flag_Z)|
                            (cpu->data & qe6502_flag_N));

    request_write(cpu, cpu->address, OFFSETOF(data));
    return resume_to(nmos_fetch_opcode);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_ROR_accumulator( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    uint8_t carry = cpu->A & 1;
    cpu->A >>= 1;
    cpu->A |= (cpu->P << 7);
    update_flags(cpu, qe6502_flag_C|qe6502_flag_Z|qe6502_flag_N,
                            carry  |
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      RTI
 *
 *  Description: Return from Interrupt
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x40( 64)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/

INSTR_FW_DECL(nmos_instr_RTI_2);
INSTR_FW_DECL(nmos_instr_RTI_3);
INSTR_FW_DECL(nmos_instr_RTI_4);
INSTR_FW_DECL(nmos_instr_RTI_5);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_RTI_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTI_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_RTI_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTI_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(P));
    return resume_to(nmos_instr_RTI_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTI_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_UN;
    cpu->P &= ~qe6502_flag_B;
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_lsb));
    return resume_to(nmos_instr_RTI_5);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTI_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_msb));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      RTS
 *
 *  Description: Return from Subroutine
 *
 *  Class: Special
 *  Variants:
 *  Opcode: 0x60( 96)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_sp_implied
 *
 *
 ********************************************************/


INSTR_FW_DECL(nmos_instr_RTS_2);
INSTR_FW_DECL(nmos_instr_RTS_3);
INSTR_FW_DECL(nmos_instr_RTS_4);
INSTR_FW_DECL(nmos_instr_RTS_5);

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_RTS_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTS_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_stack_read_dummy(cpu, cpu->S, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_instr_RTS_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTS_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_lsb));
    return resume_to(nmos_instr_RTS_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTS_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S++;
    request_stack_read(cpu, cpu->S, OFFSETOF(PC.u8_msb));
    return resume_to(nmos_instr_RTS_5);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_RTS_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    cpu->PC.u16++;
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      SBC
 *
 *  Description: Subtract with Carry
 *
 *  Class: Reader
 *  Variants:
 *  Opcode: 0xE1(225)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_r_indexed_x
 *
 *  Opcode: 0xE5(229)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_r_zeropage
 *
 *  Opcode: 0xE9(233)  Bytes: 2,  Mode: Immediate
 *          See also:  nmos_pre_r_immediate
 *
 *  Opcode: 0xED(237)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_r_absolute
 *
 *  Opcode: 0xF1(241)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_r_indexed_y
 *
 *  Opcode: 0xF5(245)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_r_zeropage_x
 *
 *  Opcode: 0xF9(249)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_r_absolute_y
 *
 *  Opcode: 0xFD(253)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_r_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SBC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    if (QE_LIKELY( 0 == (cpu->P & qe6502_flag_D) ))
    {
        return jump_to(cpu, nmos_instr_SBC_impl_bin);
    }
    else
    {
        if (cpu->model == qe6502_nes)
        {
            return jump_to(cpu, nmos_instr_SBC_impl_bin);
        }
        else
        {
            return jump_to(cpu, nmos_instr_SBC_impl_dec);
        }
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
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SEC( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_C;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      SED
 *
 *  Description: Set Decimal Flag
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xF8(248)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SED( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_D;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      SEI
 *
 *  Description: Set Interrupt Disable
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x78(120)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_SEI( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->P |= qe6502_flag_I;
    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      STA
 *
 *  Description: Store Accumulator
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x81(129)  Bytes: 2,  Mode: Indexed_X
 *          See also:  nmos_pre_w_indexed_x
 *
 *  Opcode: 0x85(133)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_w_zeropage
 *
 *  Opcode: 0x8D(141)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_w_absolute
 *
 *  Opcode: 0x91(145)  Bytes: 2,  Mode: Indexed_Y
 *          See also:  nmos_pre_w_indexed_y
 *
 *  Opcode: 0x95(149)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_w_zeropage_x
 *
 *  Opcode: 0x99(153)  Bytes: 3,  Mode: Absolute_Y
 *          See also:  nmos_pre_w_absolute_y
 *
 *  Opcode: 0x9D(157)  Bytes: 3,  Mode: Absolute_X
 *          See also:  nmos_pre_w_absolute_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_STA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(A));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      STX
 *
 *  Description: Store X Register
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x86(134)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_w_zeropage
 *
 *  Opcode: 0x8E(142)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_w_absolute
 *
 *  Opcode: 0x96(150)  Bytes: 2,  Mode: ZeroPage_Y
 *          See also:  nmos_pre_w_zeropage_y
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_STX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(X));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      STY
 *
 *  Description: Store Y Register
 *
 *  Class: Writer
 *  Variants:
 *  Opcode: 0x84(132)  Bytes: 2,  Mode: ZeroPage
 *          See also:  nmos_pre_w_zeropage
 *
 *  Opcode: 0x8C(140)  Bytes: 3,  Mode: Absolute
 *          See also:  nmos_pre_w_absolute
 *
 *  Opcode: 0x94(148)  Bytes: 2,  Mode: ZeroPage_X
 *          See also:  nmos_pre_w_zeropage_x
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_STY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write(cpu, cpu->address, OFFSETOF(Y));
    return resume_to(nmos_fetch_opcode);
}

/********************************************************
 *                      TAXnmos_pre_w_zeropage_2
 *
 *  Description: Transfer Accumulator to X
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xAA(170)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TAX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->A;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      TAY
 *
 *  Description: Transfer Accumulator to Y
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xA8(168)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TAY( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->Y = cpu->A;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->A?0:qe6502_flag_Z)|
                            (cpu->A & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      TSX
 *
 *  Description: Transfer Stack Pointer to X
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0xBA(186)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TSX( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->X = cpu->S;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->S?0:qe6502_flag_Z)|
                            (cpu->S & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}
/********************************************************
 *                      TXA
 *
 *  Description: Transfer X to Accumulator
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x8A(138)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TXA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->X;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->X?0:qe6502_flag_Z)|
                            (cpu->X & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      TXS
 *
 *  Description: Transfer X to Stack Pointer
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x9A(154)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TXS( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->S = cpu->X;

    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/********************************************************
 *                      TYA
 *
 *  Description: Transfer Y to Accumulator
 *
 *  Class: Regs
 *  Variants:
 *  Opcode: 0x98(152)  Bytes: 1,  Mode: Implied
 *          See also:  nmos_pre_rgs_implied
 *
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_instr_TYA( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->A = cpu->Y;
    update_flags(cpu, qe6502_flag_Z|qe6502_flag_N,
                            (cpu->Y?0:qe6502_flag_Z)|
                            (cpu->Y & qe6502_flag_N));


    request_read_dummy(cpu, cpu->PC, OFFSETOF(data));
    return resume_to_dummy_read(cpu, nmos_fetch_opcode);
}

/////////////////////////////////////////////////////////////////////////////////////

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: Jmp
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_jmp_absolute_2 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_jmp_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
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

INSTR_FW_DECL( nmos_pre_r_absolute_2 );
INSTR_FW_DECL( nmos_pre_r_absolute_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_r_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    return resume_to( nmos_pre_r_absolute_3 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_rw_absolute_2 );
INSTR_FW_DECL( nmos_pre_rw_absolute_3 );
INSTR_FW_DECL( nmos_pre_rw_absolute_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_rw_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    return resume_to( nmos_pre_rw_absolute_3 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( nmos_pre_rw_absolute_4 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_write( cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_w_absolute_2 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_w_absolute_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
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

INSTR_FW_DECL( nmos_pre_r_absolute_x_2 );
INSTR_FW_DECL( nmos_pre_r_absolute_x_3 );
INSTR_FW_DECL( nmos_pre_r_absolute_x_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_r_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_r_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    if (cpu->pagecross_overflow)
    {
        cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

        return resume_to_dummy_read(cpu, nmos_pre_r_absolute_x_4 );
    }
    else
    {
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_rw_absolute_x_2 );
INSTR_FW_DECL( nmos_pre_rw_absolute_x_3 );
INSTR_FW_DECL( nmos_pre_rw_absolute_x_4 );
INSTR_FW_DECL( nmos_pre_rw_absolute_x_5 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;

    return resume_to(nmos_pre_rw_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_rw_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

    return resume_to_dummy_read(cpu, nmos_pre_rw_absolute_x_4 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( nmos_pre_rw_absolute_x_5 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_x_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_write(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_X
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_w_absolute_x_2 );
INSTR_FW_DECL( nmos_pre_w_absolute_x_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_w_absolute_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->X;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_w_absolute_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_r_absolute_y_2 );
INSTR_FW_DECL( nmos_pre_r_absolute_y_3 );
INSTR_FW_DECL( nmos_pre_r_absolute_y_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;

    return resume_to(nmos_pre_r_absolute_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_r_absolute_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    if (cpu->pagecross_overflow)
    {
        cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

        return resume_to_dummy_read(cpu, nmos_pre_r_absolute_y_4 );
    }
    else
    {
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_absolute_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_Y
 *
 *  Class: ReaderWriter
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_rw_absolute_y_2 );
INSTR_FW_DECL( nmos_pre_rw_absolute_y_3 );
INSTR_FW_DECL( nmos_pre_rw_absolute_y_4 );
INSTR_FW_DECL( nmos_pre_rw_absolute_y_5 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_rw_absolute_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_rw_absolute_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

    return resume_to_dummy_read(cpu, nmos_pre_rw_absolute_y_4 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    return resume_to( nmos_pre_rw_absolute_y_5 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_absolute_y_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write_dummy(cpu, cpu->address, OFFSETOF(data));
    return resume_to_dummy_write(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Absolute_Y
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_w_absolute_y_2 );
INSTR_FW_DECL( nmos_pre_w_absolute_y_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_w_absolute_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_msb));
    cpu->PC.u16++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_w_absolute_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_absolute_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

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
nmos_pre_r_immediate( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_r_indexed_x_2 );
INSTR_FW_DECL( nmos_pre_r_indexed_x_3 );
INSTR_FW_DECL( nmos_pre_r_indexed_x_4 );
INSTR_FW_DECL( nmos_pre_r_indexed_x_5 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_lsb));
    cpu->PC.u16++;
    cpu->pointer.u8_msb = 0;

    return resume_to(nmos_pre_r_indexed_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));

    cpu->pointer.u8_lsb += cpu->X;

    return resume_to_dummy_read(cpu, nmos_pre_r_indexed_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));

    cpu->pointer.u8_lsb++;

    return resume_to(nmos_pre_r_indexed_x_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_msb));

    return resume_to( nmos_pre_r_indexed_x_5 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_x_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_w_indexed_x_2 );
INSTR_FW_DECL( nmos_pre_w_indexed_x_3 );
INSTR_FW_DECL( nmos_pre_w_indexed_x_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_w_indexed_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_msb = 0;
    request_read_dummy(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));

    cpu->pointer.u8_lsb += cpu->X;

    return resume_to_dummy_read(cpu, nmos_pre_w_indexed_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));

    cpu->pointer.u8_lsb++;

    return resume_to( nmos_pre_w_indexed_x_4 );
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_msb));

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indexed_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_r_indexed_y_2 );
INSTR_FW_DECL( nmos_pre_r_indexed_y_3 );
INSTR_FW_DECL( nmos_pre_r_indexed_y_4 );
INSTR_FW_DECL( nmos_pre_r_indexed_y_5 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_r_indexed_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_msb = 0;
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));

    return resume_to(nmos_pre_r_indexed_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_lsb++;

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_msb));

    return resume_to(nmos_pre_r_indexed_y_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    if (cpu->pagecross_overflow)
    {
        cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

        return resume_to_dummy_read(cpu, nmos_pre_r_indexed_y_5 );
    }
    else
    {
        return resume_to( cpu->instr );
    }
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_indexed_y_5( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_w_indexed_y_2 );
INSTR_FW_DECL( nmos_pre_w_indexed_y_3 );
INSTR_FW_DECL( nmos_pre_w_indexed_y_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_w_indexed_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->pointer.u8_msb = 0;
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_lsb));
    cpu->pointer.u8_lsb++;
    return resume_to(nmos_pre_w_indexed_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer,  OFFSETOF(address.u8_msb));

    cpu->address.u8_msb = 0;
    cpu->address.u16 += cpu->Y;
    cpu->pagecross_overflow = QE_S8(cpu->address.u8_msb);

    return resume_to(nmos_pre_w_indexed_y_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_indexed_y_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_msb = QE_U8(cpu->address.u8_msb + cpu->pagecross_overflow);

    return resume_to_dummy_read(cpu, cpu->instr );
}

/********************************************************
 *
 *  Mode:  Indirect
 *
 *  Class: Jmp
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_jmp_indirect_2 );
INSTR_FW_DECL( nmos_pre_jmp_indirect_3 );
INSTR_FW_DECL( nmos_pre_jmp_indirect_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_indirect( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_lsb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_jmp_indirect_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_indirect_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(pointer.u8_msb));
    cpu->PC.u16++;

    return resume_to(nmos_pre_jmp_indirect_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_indirect_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_lsb));

    // An original 6502 has does not correctly fetch the target
    // address if the indirect vector falls on a page boundary
    // (e.g. $xxFF where xx is any value from $00 to $FF).
    // In this case fetches the LSB from $xxFF as expected
    // but takes the MSB from $xx00.
    // This is fixed in some later chips like the 65SC02
    cpu->pointer.u8_lsb++;

    return resume_to(nmos_pre_jmp_indirect_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_jmp_indirect_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->pointer, OFFSETOF(address.u8_msb));

    return resume_to(nmos_instr_JMP);
}

/********************************************************
 *
 *  Mode:  ZeroPage
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_r_zeropage_2 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    cpu->address.u8_msb = 0;
    return resume_to(nmos_pre_r_zeropage_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_rw_zeropage_2 );
INSTR_FW_DECL( nmos_pre_rw_zeropage_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to(nmos_pre_rw_zeropage_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    return resume_to(nmos_pre_rw_zeropage_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write_dummy(cpu, cpu->address, OFFSETOF(data));

    return resume_to_dummy_write(cpu, cpu->instr);
}

/********************************************************
 *
 *  Mode:  ZeroPage
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_zeropage( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to( cpu->instr );
}

/********************************************************
 *
 *  Mode:  ZeroPage_X
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_r_zeropage_x_2 );
INSTR_FW_DECL( nmos_pre_r_zeropage_x_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to(nmos_pre_r_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_lsb += cpu->X;

    return resume_to_dummy_read(cpu, nmos_pre_r_zeropage_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_rw_zeropage_x_2 );
INSTR_FW_DECL( nmos_pre_rw_zeropage_x_3 );
INSTR_FW_DECL( nmos_pre_rw_zeropage_x_4 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to(nmos_pre_rw_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_lsb += cpu->X;

    return resume_to_dummy_read(cpu, nmos_pre_rw_zeropage_x_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_x_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));

    return resume_to(nmos_pre_rw_zeropage_x_4);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_rw_zeropage_x_4( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_write_dummy(cpu, cpu->address, OFFSETOF(data));

    return resume_to_dummy_write(cpu, cpu->instr);
}

/********************************************************
 *
 *  Mode:  ZeroPage_X
 *
 *  Class: Writer
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_w_zeropage_x_2 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_zeropage_x( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));
    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to(nmos_pre_w_zeropage_x_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_zeropage_x_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->address, OFFSETOF(data));
    cpu->address.u8_lsb += cpu->X;

    return resume_to(cpu->instr);
}

/********************************************************
 *
 *  Mode:  ZeroPage_Y
 *
 *  Class: Reader
 *
 ********************************************************/

INSTR_FW_DECL( nmos_pre_r_zeropage_y_2 );
INSTR_FW_DECL( nmos_pre_r_zeropage_y_3 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;

    return resume_to(nmos_pre_r_zeropage_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    cpu->address.u8_msb = 0;
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_lsb += cpu->Y;

    return resume_to_dummy_read(cpu, nmos_pre_r_zeropage_y_3);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_r_zeropage_y_3( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
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

INSTR_FW_DECL( nmos_pre_w_zeropage_y_2 );

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_zeropage_y( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read(cpu, cpu->PC, OFFSETOF(address.u8_lsb));

    cpu->PC.u16++;
    cpu->address.u8_msb = 0;

    return resume_to(nmos_pre_w_zeropage_y_2);
}

INSTR_RETTYPE qe6502_cycle_t
nmos_pre_w_zeropage_y_2( INSTR_ARGS qe6502_t* QE_RESTRICT cpu )
{
    request_read_dummy(cpu, cpu->address, OFFSETOF(data));

    cpu->address.u8_lsb += cpu->Y;

    return resume_to_dummy_read(cpu, cpu->instr);
}

#endif // QE_ENABLE_NMOS_6502
