#include "qe6502.h"
#include <stdarg.h>
#include <qe/impl_utils.h>

static const uint8_t flag_C  = qe6502_flag_C ;
static const uint8_t flag_Z  = qe6502_flag_Z ;
static const uint8_t flag_I  = qe6502_flag_I ;
static const uint8_t flag_D  = qe6502_flag_D ;
static const uint8_t flag_B  = qe6502_flag_B ;
static const uint8_t flag_UN = qe6502_flag_UN;
static const uint8_t flag_V  = qe6502_flag_V ;
static const uint8_t flag_N  = qe6502_flag_N ;

QE_MAYBE_UNUSED static inline uint8_t flag_C_if(bool cond)  {return cond ? flag_C : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_Z_if(bool cond)  {return cond ? flag_Z : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_I_if(bool cond)  {return cond ? flag_I : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_D_if(bool cond)  {return cond ? flag_D : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_B_if(bool cond)  {return cond ? flag_B : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_UN_if(bool cond) {return cond ? flag_UN : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_V_if(bool cond)  {return cond ? flag_V : 0;}
QE_MAYBE_UNUSED static inline uint8_t flag_N_if(bool cond)  {return cond ? flag_N : 0;}

QE_MAYBE_UNUSED static const uint8_t writing        = qe6502_status_writing;
QE_MAYBE_UNUSED static const uint8_t instr_done     = qe6502_status_instr_done;
QE_MAYBE_UNUSED static const uint8_t nmi_starts     = qe6502_status_nmi_starts;
QE_MAYBE_UNUSED static const uint8_t irq_starts     = qe6502_status_irq_starts;
QE_MAYBE_UNUSED static const uint8_t halted         = qe6502_status_halted;

QE_MAYBE_UNUSED
static inline void set_latch_addr0(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = qe_u16_set_byte(cpu->latch_addr, 0, value);
}

static inline void set_latch_addr1(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = qe_u16_set_byte(cpu->latch_addr, 1, value);
}

static inline qe6502_tick_t read(const qe6502_t* cpu, uint16_t address)
{
    return (qe6502_tick_t){
        .address = address,
        .status = (uint8_t)(cpu->status & (~qe6502_status_writing))
    };
}

static inline qe6502_tick_t write(const qe6502_t* cpu, uint16_t address, uint8_t data)
{
    return (qe6502_tick_t){
        .address = address,
        .bus = data,
        .status = (uint8_t)(cpu->status | qe6502_status_writing)
    };
}

static inline qe6502_tick_t stack_write(qe6502_t* cpu, uint8_t data)
{
    uint16_t address = (uint16_t)(0x0100 | cpu->S);
    cpu->S--;
    return write(cpu, address, data);
}

QE_MAYBE_UNUSED
static inline qe6502_tick_t stack_read(qe6502_t* cpu)
{
    cpu->S++;
    return read(cpu, (uint16_t)(0x0100 | cpu->S));
}

qe6502_tick_t dispatcher(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)(bus << 3);
    cpu->PC++;
    return qe6502_microcode_table[cpu->microcode](cpu, bus);
}

static inline qe6502_tick_t fetch_opcode(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    // TODO: Process interrupts
    qe6502_tick_t cmd = read(cpu, cpu->PC);
    cmd.status |= qe6502_status_instr_done;

    return cmd;
}

static inline qe6502_tick_t read_pc(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    qe6502_tick_t cmd = read(cpu, cpu->PC);
    return cmd;
}

static inline qe6502_tick_t read_inc_pc(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    qe6502_tick_t cmd = read(cpu, cpu->PC);
    cpu->PC++;
    return cmd;
}

static inline qe6502_tick_t save_to_latch_addr0_read_inc_pc(qe6502_t* cpu, uint8_t bus)
{
    qe6502_tick_t cmd = read(cpu, cpu->PC);
    cpu->PC++;
    cpu->latch_addr = bus;

    return cmd;
}

static inline qe6502_tick_t save_to_latch_addr1_idx_x_fix_pcr_with_read(qe6502_t* cpu, uint8_t bus)
{
    qe6502_tick_t cmd = read(cpu, cpu->latch_addr + cpu->X);
    set_latch_addr1(cpu, bus);
    cpu->latch_addr += cpu->X;

    return cmd;
}

static inline qe6502_tick_t r_indexed_x_2(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    qe6502_tick_t cmd = read(cpu, cpu->latch_addr);

    return cmd;
}

static inline qe6502_tick_t read_latch_addr(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    qe6502_tick_t cmd = read(cpu, cpu->latch_addr);

    return cmd;
}

static inline qe6502_tick_t write_back_bus_to_latch_addr_save_to_latch(qe6502_t* cpu, uint8_t bus)
{
    qe6502_tick_t cmd = write(cpu, cpu->latch_addr, bus);
    cpu->latch_data = bus;

    return cmd;
}

static inline qe6502_tick_t write_pc0_to_stack(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    return stack_write(cpu, (uint8_t)cpu->PC);
}

static inline qe6502_tick_t write_pc1_to_stack(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    return stack_write(cpu, (uint8_t)(cpu->PC >> 8));
}

static inline qe6502_tick_t write_p_with_B_flag_to_stack(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    return stack_write(cpu, cpu->P | flag_B);
}

static inline qe6502_tick_t read_brk_vector0(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    return read(cpu, 0xfffe);
}

static inline qe6502_tick_t save_pc0_read_brk_vector1(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    cpu->PC = bus;
    return read(cpu, 0xffff);
}

///////////////

static inline qe6502_tick_t op_ILL(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    cpu->status = qe6502_error_illegal_op;
    cpu->status |= halted;
    cpu->microcode--;
    return read(cpu, cpu->PC);
}

static inline qe6502_tick_t save_pc1_op_BRK_fetch(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    cpu->PC |= (uint16_t)(bus << 8);
    cpu->P |= flag_I;
    return fetch_opcode(cpu, bus);
}


static inline qe6502_tick_t op_ROR(qe6502_t* cpu, QE_MAYBE_UNUSED uint8_t bus)
{
    uint8_t data = cpu->latch_data;
    uint8_t carry = qe_u8_bit(data, 0);
    data >>= 1;
    data |= QE_U8(cpu->P << 7);
    cpu->P = qe_u8_set_mask(cpu->P,
                            flag_C|flag_Z|flag_N,
                            carry                           |
                            flag_Z_if(cpu->latch_data == 0) |
                            (cpu->latch_data & flag_N));

    return write(cpu, cpu->latch_addr, data);
}

#define IDX(opcode, microcode) (((opcode) << 3) + (microcode))

const qe6502_microcode_fn qe6502_microcode_table[(256+16)*8] = {

    // BRK Implied, class:Interrupt
    [IDX(0x00, 0)] = &read_inc_pc,
    [IDX(0x00, 1)] = &write_pc1_to_stack,
    [IDX(0x00, 2)] = &write_pc0_to_stack,
    [IDX(0x00, 3)] = &write_p_with_B_flag_to_stack,
    [IDX(0x00, 4)] = &read_brk_vector0,
    [IDX(0x00, 5)] = &save_pc0_read_brk_vector1,
    [IDX(0x00, 6)] = &save_pc1_op_BRK_fetch,
    [IDX(0x00, 7)] = &dispatcher,

    // ORA (Indirect,X), class:R
    [IDX(0x01, 0)] = &read_inc_pc,
    [IDX(0x01, 1)] = &r_indexed_x_2,
    [IDX(0x01, 2)] = &op_ILL,
    [IDX(0x01, 3)] = &op_ILL,
    [IDX(0x01, 4)] = &op_ILL,
    [IDX(0x01, 5)] = &op_ILL,
    [IDX(0x01, 6)] = &dispatcher,

    // ILL
    [IDX(0x02, 0)] = &op_ILL,

    // ROR Absolute X, class:RW
    [IDX(0x7E, 0)] = &read_inc_pc,
    [IDX(0x7E, 1)] = &save_to_latch_addr0_read_inc_pc,
    [IDX(0x7E, 2)] = &save_to_latch_addr1_idx_x_fix_pcr_with_read,
    [IDX(0x7E, 3)] = &read_latch_addr,
    [IDX(0x7E, 4)] = &write_back_bus_to_latch_addr_save_to_latch,
    [IDX(0x7E, 5)] = &op_ROR,
    [IDX(0x7E, 6)] = &fetch_opcode,
    [IDX(0x7E, 7)] = &dispatcher,

    // NOP Implied, class:0
    [IDX(0xEA, 0)] = &read_pc,
    [IDX(0xEA, 1)] = &fetch_opcode,
    [IDX(0xEA, 2)] = &dispatcher,
};
