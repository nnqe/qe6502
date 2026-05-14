// Contains cmos opcode handler declarations and metadata used by the emulator.

#ifndef QE6502_CMOS_OPCODES_H__
#define QE6502_CMOS_OPCODES_H__

#include "qe6502_cmos_fw.h"

static const qe6502_model_t cmos_model = {
.opcodes =
{
    /*----------------------------------------------------*/
    //  BRK, Force Interrupt
    //  class: Special
    //  Opcode: 0x00(  0), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_BRK, cmos_instr_BRK_finalize },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x01(  1), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x02(2  )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x03(3  )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  TSB, Test and Set Bits
    //  class: ReaderWriter
    //  Opcode: 0x04(4  ), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_TSB },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x05(  5), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ASL, Arithmetic Shift Left
    //  class: ReaderWriter
    //  Opcode: 0x06(  6), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_ASL },
    /*----------------------------------------------------*/
    //  RMB0, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x07(  7), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  PHP, Push Processor Status
    //  class: Special
    //  Opcode: 0x08(  8), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PHP, cmos_instr_PHP },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x09(  9), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ASL, Arithmetic Shift Left
    //  class: ReaderWriter
    //  Opcode: 0x0A( 10), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_ASL_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x0B(11 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  TSB, Test and Set Bits
    //  class: ReaderWriter
    //  Opcode: 0x0C(12 ), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_TSB },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x0D( 13), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ASL, Arithmetic Shift Left
    //  class: ReaderWriter
    //  Opcode: 0x0E( 14), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_ASL },
    /*----------------------------------------------------*/
    //  BBR0, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x0F(15 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BPL, Branch if Positive
    //  class: BranchIf
    //  Opcode: 0x10( 16), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BPL, cmos_instr_BPL },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x11( 17), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x12(18 ), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x13(19 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  TRB, Test and Reset Bits
    //  class: ReaderWriter
    //  Opcode: 0x14(20 ), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_TRB },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x15( 21), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ASL, Arithmetic Shift Left
    //  class: ReaderWriter
    //  Opcode: 0x16( 22), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_ASL },
    /*----------------------------------------------------*/
    //  RMB1, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x17( 23), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  CLC, Clear Carry Flag
    //  class: Regs
    //  Opcode: 0x18( 24), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_CLC, cmos_instr_CLC },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x19( 25), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  INC, Increment Accumulator
    //  class: ReaderWriter
    //  Opcode: 0x1A(26 ), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_INC_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x1B(27 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  TRB, Test and Reset Bits
    //  class: ReaderWriter
    //  Opcode: 0x1C(28 ), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_TRB },
    /*----------------------------------------------------*/
    //  ORA, Logical Inclusive OR
    //  class: Reader
    //  Opcode: 0x1D( 29), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_ORA },
    /*----------------------------------------------------*/
    //  ASL, Arithmetic Shift Left
    //  class: ReaderWriter
    //  Opcode: 0x1E( 30), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x_short, cmos_instr_ASL },
    /*----------------------------------------------------*/
    //  BBR1, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x1F(31 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  JSR, Jump to Subroutine
    //  class: Special
    //  Opcode: 0x20( 32), Bytes: 3, Mode: Absolute
    //
    {   cmos_instr_JSR, cmos_instr_JSR },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x21( 33), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x22(34 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x23(35 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  BIT, Bit Test
    //  class: Reader
    //  Opcode: 0x24( 36), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_BIT },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x25( 37), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  ROL, Rotate Left
    //  class: ReaderWriter
    //  Opcode: 0x26( 38), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_ROL },
    /*----------------------------------------------------*/
    //  RMB2, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x27( 39), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  PLP, Pull Processor Status
    //  class: Special
    //  Opcode: 0x28( 40), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PLP, cmos_instr_PLP },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x29( 41), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  ROL, Rotate Left
    //  class: ReaderWriter
    //  Opcode: 0x2A( 42), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_ROL_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x2B(43 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  BIT, Bit Test
    //  class: Reader
    //  Opcode: 0x2C( 44), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_BIT },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x2D( 45), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  ROL, Rotate Left
    //  class: ReaderWriter
    //  Opcode: 0x2E( 46), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_ROL },
    /*----------------------------------------------------*/
    //  BBR2, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x2F(47 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BMI, Branch if Minus
    //  class: BranchIf
    //  Opcode: 0x30( 48), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BMI, cmos_instr_BMI },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x31( 49), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x32(50 ), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x33(51 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  BIT, Bit Test
    //  class: Reader
    //  Opcode: 0x34(52 ), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_BIT },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x35( 53), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  ROL, Rotate Left
    //  class: ReaderWriter
    //  Opcode: 0x36( 54), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_ROL },
    /*----------------------------------------------------*/
    //  RMB3, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x37( 55), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  SEC, Set Carry Flag
    //  class: Regs
    //  Opcode: 0x38( 56), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_SEC, cmos_instr_SEC },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x39( 57), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  DEC, Decrement Accumulator
    //  class: ReaderWriter
    //  Opcode: 0x3A(58 ), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_DEC_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x3B(59 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  BIT, Bit Test
    //  class: Reader
    //  Opcode: 0x3C(60 ), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_BIT },
    /*----------------------------------------------------*/
    //  AND, Logical AND
    //  class: Reader
    //  Opcode: 0x3D( 61), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_AND },
    /*----------------------------------------------------*/
    //  ROL, Rotate Left
    //  class: ReaderWriter
    //  Opcode: 0x3E( 62), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x_short, cmos_instr_ROL },
    /*----------------------------------------------------*/
    //  BBR3, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x3F(63 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  RTI, Return from Interrupt
    //  class: Special
    //  Opcode: 0x40( 64), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_RTI, cmos_instr_RTI },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x41( 65), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x42(66 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x43(67 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x44(68 )
    //
    {   cmos_pre_r_zeropage, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x45( 69), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  LSR, Logical Shift Right
    //  class: ReaderWriter
    //  Opcode: 0x46( 70), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_LSR },
    /*----------------------------------------------------*/
    //  RMB4, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x47( 71), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  PHA, Push Accumulator
    //  class: Special
    //  Opcode: 0x48( 72), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PHA, cmos_instr_PHA },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x49( 73), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  LSR, Logical Shift Right
    //  class: ReaderWriter
    //  Opcode: 0x4A( 74), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_LSR_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x4B(75 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  JMP, Jump
    //  class: Jmp
    //  Opcode: 0x4C( 76), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_jmp_absolute, cmos_instr_JMP },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x4D( 77), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  LSR, Logical Shift Right
    //  class: ReaderWriter
    //  Opcode: 0x4E( 78), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_LSR },
    /*----------------------------------------------------*/
    //  BBR4, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x4F(79 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BVC, Branch if Overflow Clear
    //  class: BranchIf
    //  Opcode: 0x50( 80), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BVC, cmos_instr_BVC },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x51( 81), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x52(82 ), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x53(83 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x54(84 )
    //
    {   cmos_pre_r_zeropage_x, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x55( 85), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  LSR, Logical Shift Right
    //  class: ReaderWriter
    //  Opcode: 0x56( 86), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_LSR },
    /*----------------------------------------------------*/
    //  RMB5, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x57( 87), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  CLI, Clear Interrupt Disable
    //  class: Regs
    //  Opcode: 0x58( 88), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_CLI, cmos_instr_CLI },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x59( 89), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  PHY, Push Register Y
    //  class: Special
    //  Opcode: 0x5A(90 ), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PHY, cmos_instr_PHY },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x5B(91 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x5C(92 )
    //
    {   cmos_instr_0x5C_0xDC_0xFC, QE_NULL },
    /*----------------------------------------------------*/
    //  EOR, Exclusive OR
    //  class: Reader
    //  Opcode: 0x5D( 93), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_EOR },
    /*----------------------------------------------------*/
    //  LSR, Logical Shift Right
    //  class: ReaderWriter
    //  Opcode: 0x5E( 94), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x_short, cmos_instr_LSR },
    /*----------------------------------------------------*/
    //  BBR5, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x5F(95 ), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  RTS, Return from Subroutine
    //  class: Special
    //  Opcode: 0x60( 96), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_RTS, cmos_instr_RTS },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x61( 97), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x62(98 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x63(99 )
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STZ, Store Zero
    //  class: Writer
    //  Opcode: 0x64(100), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_w_zeropage, cmos_instr_STZ },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x65(101), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  ROR, Rotate Right
    //  class: ReaderWriter
    //  Opcode: 0x66(102), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_ROR },
    /*----------------------------------------------------*/
    //  RMB6, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x67(103), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  PLA, Pull Accumulator
    //  class: Special
    //  Opcode: 0x68(104), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PLA, cmos_instr_PLA },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x69(105), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_ADC_Immediate },
    /*----------------------------------------------------*/
    //  ROR, Rotate Right
    //  class: ReaderWriter
    //  Opcode: 0x6A(106), Bytes: 1, Mode: Accumulator
    //
    {   cmos_instr_ROR_accumulator, QE_NULL },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x6B(107)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  JMP, Jump
    //  class: Jmp
    //  Opcode: 0x6C(108), Bytes: 3, Mode: Indirect
    //
    {   cmos_pre_jmp_indirect, cmos_instr_JMP },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x6D(109), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  ROR, Rotate Right
    //  class: ReaderWriter
    //  Opcode: 0x6E(110), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_ROR },
    /*----------------------------------------------------*/
    //  BBR6, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x6F(111), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BVS, Branch if Overflow Set
    //  class: BranchIf
    //  Opcode: 0x70(112), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BVS, cmos_instr_BVS },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x71(113), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x72(114), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x73(115)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STZ, Store Zero
    //  class: Writer
    //  Opcode: 0x74(116), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_w_zeropage_x, cmos_instr_STZ },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x75(117), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  ROR, Rotate Right
    //  class: ReaderWriter
    //  Opcode: 0x76(118), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_ROR },
    /*----------------------------------------------------*/
    //  RMB7, Reset Bit
    //  class: ReaderWriter
    //  Opcode: 0x77(119), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  SEI, Set Interrupt Disable
    //  class: Regs
    //  Opcode: 0x78(120), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_SEI, cmos_instr_SEI },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x79(121), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  PLY, Pull Register Y
    //  class: Special
    //  Opcode: 0x7A(122), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PLY, cmos_instr_PLY },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x7B(123)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  JMP, Jump
    //  class: Jmp
    //  Opcode: 0x7C(124), Bytes: 3, Mode: Indirect_X
    //
    {   cmos_pre_jmp_indirect_x, cmos_pre_jmp_indirect_x },
    /*----------------------------------------------------*/
    //  ADC, Add with Carry
    //  class: Reader
    //  Opcode: 0x7D(125), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_ADC },
    /*----------------------------------------------------*/
    //  ROR, Rotate Right
    //  class: ReaderWriter
    //  Opcode: 0x7E(126), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x_short, cmos_instr_ROR },
    /*----------------------------------------------------*/
    //  BBR7, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x7F(127), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbr_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BRA, Branch if Always
    //  class: BranchIf
    //  Opcode: 0x80(128), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BRA, cmos_instr_BRA },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x81(129), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_w_indexed_x, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x82(130)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x83(131)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STY, Store Y Register
    //  class: Writer
    //  Opcode: 0x84(132), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_w_zeropage, cmos_instr_STY },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x85(133), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_w_zeropage, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  STX, Store X Register
    //  class: Writer
    //  Opcode: 0x86(134), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_w_zeropage, cmos_instr_STX },
    /*----------------------------------------------------*/
    //  SMB0, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0x87(135), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  DEY, Decrement Y Register
    //  class: Regs
    //  Opcode: 0x88(136), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_DEY, cmos_instr_DEY },
    /*----------------------------------------------------*/
    //  BIT, Bit Test
    //  class: Reader
    //  Opcode: 0x89(137), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_BIT_immediate },
    /*----------------------------------------------------*/
    //  TXA, Transfer X to Accumulator
    //  class: Regs
    //  Opcode: 0x8A(138), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TXA, cmos_instr_TXA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x8B(139)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STY, Store Y Register
    //  class: Writer
    //  Opcode: 0x8C(140), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_w_absolute, cmos_instr_STY },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x8D(141), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_w_absolute, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  STX, Store X Register
    //  class: Writer
    //  Opcode: 0x8E(142), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_w_absolute, cmos_instr_STX },
    /*----------------------------------------------------*/
    //  BBS0, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x8F(143), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BCC, Branch if Carry Clear
    //  class: BranchIf
    //  Opcode: 0x90(144), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BCC, cmos_instr_BCC },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x91(145), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_w_indexed_y, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x92(146), Byte: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_w_indirect_zp, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x93(147)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STY, Store Y Register
    //  class: Writer
    //  Opcode: 0x94(148), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_w_zeropage_x, cmos_instr_STY },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x95(149), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_w_zeropage_x, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  STX, Store X Register
    //  class: Writer
    //  Opcode: 0x96(150), Bytes: 2, Mode: ZeroPage_Y
    //
    {   cmos_pre_w_zeropage_y, cmos_instr_STX },
    /*----------------------------------------------------*/
    //  SMB1, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0x97(151), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  TYA, Transfer Y to Accumulator
    //  class: Regs
    //  Opcode: 0x98(152), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TYA, cmos_instr_TYA },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x99(153), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_w_absolute_y, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  TXS, Transfer X to Stack Pointer
    //  class: Regs
    //  Opcode: 0x9A(154), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TXS, cmos_instr_TXS },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0x9B(155)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  STZ, Store Zero
    //  class: Writer
    //  Opcode: 0x9C(156), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_w_absolute, cmos_instr_STZ },
    /*----------------------------------------------------*/
    //  STA, Store Accumulator
    //  class: Writer
    //  Opcode: 0x9D(157), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_w_absolute_x, cmos_instr_STA },
    /*----------------------------------------------------*/
    //  STZ, Store Zero
    //  class: Writer
    //  Opcode: 0x9E(158), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_w_absolute_x, cmos_instr_STZ },
    /*----------------------------------------------------*/
    //  BBS1, Branch on Bit Reset
    //  class: Reader
    //  Opcode: 0x9F(159), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  LDY, Load Y Register
    //  class: Reader
    //  Opcode: 0xA0(160), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_LDY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xA1(161), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDX, Load X Register
    //  class: Reader
    //  Opcode: 0xA2(162), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_LDX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xA3(163)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  LDY, Load Y Register
    //  class: Reader
    //  Opcode: 0xA4(164), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_LDY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xA5(165), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDX, Load X Register
    //  class: Reader
    //  Opcode: 0xA6(166), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_LDX },
    /*----------------------------------------------------*/
    //  SMB2, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xA7(167), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  TAY, Transfer Accumulator to Y
    //  class: Regs
    //  Opcode: 0xA8(168), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TAY, cmos_instr_TAY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xA9(169), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  TAX, Transfer Accumulator to X
    //  class: Regs
    //  Opcode: 0xAA(170), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TAX, cmos_instr_TAX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xAB(171)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  LDY, Load Y Register
    //  class: Reader
    //  Opcode: 0xAC(172), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_LDY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xAD(173), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDX, Load X Register
    //  class: Reader
    //  Opcode: 0xAE(174), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_LDX },
    /*----------------------------------------------------*/
    //  BBS2, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xAF(175), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BCS, Branch if Carry Set
    //  class: BranchIf
    //  Opcode: 0xB0(176), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BCS, cmos_instr_BCS },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xB1(177), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xB2(178), Bytes: 2, Mode: Indeirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xB3(179)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  LDY, Load Y Register
    //  class: Reader
    //  Opcode: 0xB4(180), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_LDY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xB5(181), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDX, Load X Register
    //  class: Reader
    //  Opcode: 0xB6(182), Bytes: 2, Mode: ZeroPage_Y
    //
    {   cmos_pre_r_zeropage_y, cmos_instr_LDX },
    /*----------------------------------------------------*/
    //  SMB3, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xB7(183), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  CLV, Clear Overflow Flag
    //  class: Regs
    //  Opcode: 0xB8(184), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_CLV, cmos_instr_CLV },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xB9(185), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  TSX, Transfer Stack Pointer to X
    //  class: Regs
    //  Opcode: 0xBA(186), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_TSX, cmos_instr_TSX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xBB(187)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  LDY, Load Y Register
    //  class: Reader
    //  Opcode: 0xBC(188), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_LDY },
    /*----------------------------------------------------*/
    //  LDA, Load Accumulator
    //  class: Reader
    //  Opcode: 0xBD(189), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_LDA },
    /*----------------------------------------------------*/
    //  LDX, Load X Register
    //  class: Reader
    //  Opcode: 0xBE(190), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_LDX },
    /*----------------------------------------------------*/
    //  BBS3, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xBF(191), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  CPY, Compare Y Register
    //  class: Reader
    //  Opcode: 0xC0(192), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_CPY },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xC1(193), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xC2(194)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xC3(195)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  CPY, Compare Y Register
    //  class: Reader
    //  Opcode: 0xC4(196), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_CPY },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xC5(197), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  DEC, Decrement Memory
    //  class: ReaderWriter
    //  Opcode: 0xC6(198), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_DEC },
    /*----------------------------------------------------*/
    //  SMB4, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xC7(199), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  INY, Increment Y Register
    //  class: Regs
    //  Opcode: 0xC8(200), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_INY, cmos_instr_INY },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xC9(201), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  DEX, Decrement X Register
    //  class: Regs
    //  Opcode: 0xCA(202), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_DEX, cmos_instr_DEX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xCB(203)
    //
    {   cmos_instr_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  CPY, Compare Y Register
    //  class: Reader
    //  Opcode: 0xCC(204), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_CPY },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xCD(205), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  DEC, Decrement Memory
    //  class: ReaderWriter
    //  Opcode: 0xCE(206), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_DEC },
    /*----------------------------------------------------*/
    //  BBS4, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xCF(207), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BNE, Branch if Not Equal
    //  class: BranchIf
    //  Opcode: 0xD0(208), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BNE, cmos_instr_BNE },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xD1(209), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xD2(210), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xD3(211)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xD4(212)
    //
    {   cmos_pre_r_zeropage_x, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xD5(213), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  DEC, Decrement Memory
    //  class: ReaderWriter
    //  Opcode: 0xD6(214), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_DEC },
    /*----------------------------------------------------*/
    //  SMB5, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xD7(215), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  CLD, Clear Decimal Mode
    //  class: Regs
    //  Opcode: 0xD8(216), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_CLD, cmos_instr_CLD },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xD9(217), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  PHX, Push Register X
    //  class: Special
    //  Opcode: 0xDA(218), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PHX, cmos_instr_PHX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xDB(219)
    //
    {   cmos_pre_r_zeropage_x, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xDC(220)
    //
    {   cmos_instr_0x5C_0xDC_0xFC, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  CMP, Compare
    //  class: Reader
    //  Opcode: 0xDD(221), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_CMP },
    /*----------------------------------------------------*/
    //  DEC, Decrement Memory
    //  class: ReaderWriter
    //  Opcode: 0xDE(222), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x, cmos_instr_DEC },
    /*----------------------------------------------------*/
    //  BBS5, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xDF(223), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  CPX, Compare X Register
    //  class: Reader
    //  Opcode: 0xE0(224), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_CPX },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xE1(225), Bytes: 2, Mode: Indexed_X
    //
    {   cmos_pre_r_indexed_x, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xE2(226)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xE3(227)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  CPX, Compare X Register
    //  class: Reader
    //  Opcode: 0xE4(228), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_CPX },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xE5(229), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_r_zeropage, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  INC, Increment Memory
    //  class: ReaderWriter
    //  Opcode: 0xE6(230), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage, cmos_instr_INC },
    /*----------------------------------------------------*/
    //  SMB6, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xE7(231), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  INX, Increment X Register
    //  class: Regs
    //  Opcode: 0xE8(232), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_INX, cmos_instr_INX },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xE9(233), Bytes: 2, Mode: Immediate
    //
    {   cmos_pre_r_immediate, cmos_instr_SBC_Immediate },
    /*----------------------------------------------------*/
    //  NOP, No Operation
    //  class: Special
    //  Opcode: 0xEA(234), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_NOP, cmos_instr_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xEB(235)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  CPX, Compare X Register
    //  class: Reader
    //  Opcode: 0xEC(236), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_CPX },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xED(237), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_r_absolute, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  INC, Increment Memory
    //  class: ReaderWriter
    //  Opcode: 0xEE(238), Bytes: 3, Mode: Absolute
    //
    {   cmos_pre_rw_absolute, cmos_instr_INC },
    /*----------------------------------------------------*/
    //  BBS6, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xEF(239), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
    //  BEQ, Branch if Equal
    //  class: BranchIf
    //  Opcode: 0xF0(240), Bytes: 2, Mode: Relative
    //
    {   cmos_instr_BEQ, cmos_instr_BEQ },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xF1(241), Bytes: 2, Mode: Indexed_Y
    //
    {   cmos_pre_r_indexed_y, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xF2(242), Bytes: 2, Mode: Indirect_ZP
    //
    {   cmos_pre_r_indirect_zp, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xF3(243)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xF4(244)
    //
    {   cmos_pre_r_zeropage_x, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xF5(245), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_r_zeropage_x, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  INC, Increment Memory
    //  class: ReaderWriter
    //  Opcode: 0xF6(246), Bytes: 2, Mode: ZeroPage_X
    //
    {   cmos_pre_rw_zeropage_x, cmos_instr_INC },
    /*----------------------------------------------------*/
    //  SMB7, Set Bit
    //  class: ReaderWriter
    //  Opcode: 0xF7(247), Bytes: 2, Mode: ZeroPage
    //
    {   cmos_pre_rw_zeropage_RMB_SMB, cmos_instr_RMB_SMB },
    /*----------------------------------------------------*/
    //  SED, Set Decimal Flag
    //  class: Regs
    //  Opcode: 0xF8(248), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_SED, cmos_instr_SED },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xF9(249), Bytes: 3, Mode: Absolute_Y
    //
    {   cmos_pre_r_absolute_y, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  PLX, Pull Register X
    //  class: Special
    //  Opcode: 0xFA(250), Bytes: 1, Mode: Implied
    //
    {   cmos_instr_PLX, cmos_instr_PLX },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xFB(251)
    //
    {   cmos_instr_ILLEGAL_NOP, cmos_instr_ILLEGAL_NOP },
    /*----------------------------------------------------*/
    //  OPCODE INVALID
    //  Opcode: 0xFC(252)
    //
    {   cmos_instr_0x5C_0xDC_0xFC, cmos_fetch_opcode },
    /*----------------------------------------------------*/
    //  SBC, Subtract with Carry
    //  class: Reader
    //  Opcode: 0xFD(253), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_r_absolute_x, cmos_instr_SBC },
    /*----------------------------------------------------*/
    //  INC, Increment Memory
    //  class: ReaderWriter
    //  Opcode: 0xFE(254), Bytes: 3, Mode: Absolute_X
    //
    {   cmos_pre_rw_absolute_x, cmos_instr_INC },
    /*----------------------------------------------------*/
    //  BBS7, Branch on Bit Set
    //  class: Reader
    //  Opcode: 0xFF(255), Bytes: 3, Mode: ZeroPage_Relative
    //
    {   cmos_pre_rw_bbs_zeropage_relative, cmos_instr_ILLEGAL_NOP_st_xXf },
    /*----------------------------------------------------*/
}
};
#endif // QE6502_CMOS_OPCODES_H__
