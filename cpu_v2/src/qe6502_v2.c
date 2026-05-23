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
    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

qe6502_tick_t dispatcher(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)(bus << 3);
    cpu->PC++;
    return qe6502_microcode_table[cpu->microcode](cpu, bus);
}

QE_MAYBE_UNUSED
static inline void update_flags_nz(qe6502_t* cpu, uint8_t value)
{
    const uint8_t mask = (uint8_t)(flag_Z | flag_N);
    const uint8_t flags = (uint8_t)(flag_Z_if(value == 0u) | (value & flag_N));

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
}

QE_MAYBE_UNUSED
static inline void update_flags_nzc(qe6502_t* cpu, uint8_t value, uint8_t carry)
{
    const uint8_t mask = (uint8_t)(flag_C | flag_Z | flag_N);
    const uint8_t flags = (uint8_t)(
        (carry & flag_C) |
        flag_Z_if(value == 0u) |
        (value & flag_N)
        );

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
}

static inline void compare_register_with_value(qe6502_t* cpu, uint8_t reg, uint8_t value)
{
    const uint8_t result = (uint8_t)(reg - value);
    update_flags_nzc(cpu, result, flag_C_if(reg >= value));
}


/*
 * Generated QE6502 v2 NMOS skeleton.
 * Paste into cpu_v2/src/qe6502.c after the low-level helpers.
 * Replace the old experimental dispatcher/fetch/op/table block.
 * Handler bodies are placeholders; table symbols come from JSON.
 */

/* illegal_handler; role=illegal; action=placeholder illegal opcode handler */
static qe6502_tick_t op_ILL(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    (void)(flag_C | flag_Z | flag_I | flag_D | flag_B | flag_UN | flag_V | flag_N);
    cpu->status = (uint8_t)(qe6502_error_illegal_op | qe6502_status_halted);
    cpu->microcode = (uint16_t)(cpu->microcode - 1u);
    return read(cpu, cpu->PC);
}

/* shared_handler; role=dispatch; action=consume_fetched_opcode_and_dispatch_to_opcode_class */
static qe6502_tick_t mc_dispatch(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)((uint16_t)bus << 3);
    cpu->PC = (uint16_t)(cpu->PC + 1u);
    return qe6502_microcode_table[cpu->microcode](cpu, bus);
}

/* shared_handler; role=fetch; action=consume_previous_bus_cycle_and_fetch_next_opcode */
static qe6502_tick_t mc_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = (uint8_t)(tick.status | qe6502_status_instr_done);
    return tick;
}

/* shared_handler; role=fetch; action=consume_vector_high_and_fetch_next_opcode_without_interrupt_check */
static qe6502_tick_t mc_fetch_no_interrupts(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 1, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = (uint8_t)(tick.status | qe6502_status_instr_done);
    return tick;
}

/* control_handler; role=apply; action=apply_signed_offset_to_pc_low_and_request_branch_dummy_read */
static inline qe6502_tick_t mc_branch_rel_c1_apply(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t old_pc = cpu->PC;
    const int8_t offset = (int8_t)bus;
    const uint16_t target = (uint16_t)(old_pc + offset);

    qe6502_tick_t tick = read(cpu, old_pc);
    cpu->PC = qe_u16_set_byte(cpu->PC, 0, qe_u16_byte(target, 0));

    if (qe_u16_byte(old_pc, 1) == qe_u16_byte(target, 1))
    {
        cpu->microcode++;
    }
    else
    {
        cpu->latch_addr = target;
    }

    return tick;
}

/* control_handler; role=pgfix; action=perform_page_cross_branch_correction_read_and_commit_target_pc */
static inline qe6502_tick_t mc_branch_rel_c2_pgfix(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC = cpu->latch_addr;
    return tick;
}

/* special_handler; role=pad; action=dummy_read_signature_byte_and_increment_pc */
static qe6502_tick_t mc_brk_c0_pad(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=push_pch; action=push_pc_high_to_stack */
static qe6502_tick_t mc_brk_c1_push_pch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, qe_u16_byte(cpu->PC, 1));
}

/* special_handler; role=push_pcl; action=push_pc_low_to_stack */
static qe6502_tick_t mc_brk_c2_push_pcl(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, qe_u16_byte(cpu->PC, 0));
}

/* special_handler; role=push_p; action=push_status_to_stack */
static qe6502_tick_t mc_brk_c3_push_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, (uint8_t)(cpu->P | flag_B));
}

/* special_handler; role=vec_lo; action=read_brk_vector_low_to_pc_low */
static qe6502_tick_t mc_brk_c4_vec_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffeu);
}

/* special_handler; role=vec_hi; action=read_brk_vector_high_to_pc_high_and_set_interrupt_disable */
static qe6502_tick_t mc_brk_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)(cpu->P | flag_I);

    return read(cpu, 0xffffu);
}
/* special_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static inline qe6502_tick_t mc_jmp_abs_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=hi; action=read_pc_to_ea_high_and_increment_pc */
static inline qe6502_tick_t mc_jmp_abs_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=fetch; action=set_pc_to_effective_address_and_fetch_next_opcode */
static inline qe6502_tick_t mc_jmp_abs_c2_fetch(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    cpu->PC = cpu->latch_addr;

    return mc_fetch(cpu, bus);
}

/* special_handler; role=ptrlo; action=read_pc_to_pointer_low_and_increment_pc */
static inline qe6502_tick_t mc_jmp_ind_c0_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=ptrhi; action=read_pc_to_pointer_high_and_increment_pc */
static inline qe6502_tick_t mc_jmp_ind_c1_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=target_lo; action=read_pointer_to_ea_low_then_increment_pointer_low_with_nmos_wrap */
static inline qe6502_tick_t mc_jmp_ind_c2_target_lo(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(qe_u16_byte(cpu->latch_addr, 0) + 1u));
    return tick;
}

/* special_handler; role=target_hi; action=read_wrapped_pointer_to_ea_high */
static inline qe6502_tick_t mc_jmp_ind_c3_target_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return read(cpu, cpu->latch_addr);
}

/* special_handler; role=fetch; action=set_pc_to_effective_address_and_fetch_next_opcode */
static inline qe6502_tick_t mc_jmp_ind_c4_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->latch_data, 1, bus);

    return mc_fetch(cpu, bus);
}

/* special_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static inline qe6502_tick_t mc_jsr_abs_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* special_handler; role=dummy; action=dummy_stack_read_before_pushes */
static inline qe6502_tick_t mc_jsr_abs_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* special_handler; role=push_pch; action=push_return_pc_high_to_stack */
static inline qe6502_tick_t mc_jsr_abs_c2_push_pch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, qe_u16_byte(cpu->PC, 1));
}

/* special_handler; role=push_pcl; action=push_return_pc_low_to_stack */
static inline qe6502_tick_t mc_jsr_abs_c3_push_pcl(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, qe_u16_byte(cpu->PC, 0));
}

/* special_handler; role=hi; action=read_pc_to_ea_high_without_incrementing_pc */
static inline qe6502_tick_t mc_jsr_abs_c4_hi(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* special_handler; role=fetch; action=set_pc_to_effective_address_and_fetch_next_opcode */
static inline qe6502_tick_t mc_jsr_abs_c5_fetch(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    cpu->PC = cpu->latch_addr;

    return mc_fetch(cpu, bus);
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static inline qe6502_tick_t mc_r_abs_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_and_increment_pc */
static inline qe6502_tick_t mc_r_abs_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static inline qe6502_tick_t mc_r_abs_c2_data(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static inline qe6502_tick_t mc_r_abx_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_increment_pc_add_x_low_and_record_page_cross */
static inline qe6502_tick_t mc_r_abx_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=probe; action=read_indexed_probe_address_to_data_and_skip_pgfix_if_no_page_cross */
static inline qe6502_tick_t mc_r_abx_c2_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    if (page_cross == 0u)
    {
        cpu->microcode++;
    }

    return tick;
}

/* addressing_handler; role=pgfix; action=fix_ea_high_after_page_cross_and_read_effective_address_to_data */
static inline qe6502_tick_t mc_r_abx_c3_pgfix(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static inline qe6502_tick_t mc_r_aby_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_increment_pc_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_r_aby_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=probe; action=read_indexed_probe_address_to_data_and_skip_pgfix_if_no_page_cross */
static inline qe6502_tick_t mc_r_aby_c2_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    if (page_cross == 0u)
    {
        cpu->microcode++;
    }

    return tick;
}

/* addressing_handler; role=pgfix; action=fix_ea_high_after_page_cross_and_read_effective_address_to_data */
static inline qe6502_tick_t mc_r_aby_c3_pgfix(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=data; action=read_pc_to_data_and_increment_pc */
static inline qe6502_tick_t mc_r_imm_c0_data(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=ptr; action=read_pc_to_zp_pointer_and_increment_pc */
static inline qe6502_tick_t mc_r_izx_c0_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zp_pointer_then_add_x_wraparound */
static inline qe6502_tick_t mc_r_izx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));
    return tick;
}

/* addressing_handler; role=ptrlo; action=read_zp_pointer_to_ea_low_then_increment_pointer_wraparound */
static inline qe6502_tick_t mc_r_izx_c2_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t addr = cpu->latch_addr;
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(qe_u16_byte(addr, 0) + 1u));
    return tick;
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high */
static inline qe6502_tick_t mc_r_izx_c3_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static inline qe6502_tick_t mc_r_izx_c4_data(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = qe_u16_set_byte(cpu->latch_data, 1, bus);

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=ptr; action=read_pc_to_zp_pointer_and_increment_pc */
static inline qe6502_tick_t mc_r_izy_c0_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=ptrlo; action=read_zp_pointer_to_ea_low */
static inline qe6502_tick_t mc_r_izy_c1_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=ptrhi; action=increment_pointer_read_ea_high_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_r_izy_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    const uint16_t ptr_hi_addr = qe_u16_set_byte(
        cpu->latch_addr,
        0,
        (uint8_t)(qe_u16_byte(cpu->latch_addr, 0) + 1u)
        );

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=probe; action=read_indexed_probe_address_to_data_and_skip_pgfix_if_no_page_cross */
static inline qe6502_tick_t mc_r_izy_c3_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    if (page_cross == 0u)
    {
        cpu->microcode++;
    }

    return tick;
}

/* addressing_handler; role=pgfix; action=fix_ea_high_after_page_cross_and_read_effective_address_to_data */
static inline qe6502_tick_t mc_r_izy_c4_pgfix(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc_clear_ea_high */
static qe6502_tick_t mc_r_zp_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static qe6502_tick_t mc_r_zp_c1_data(qe6502_t* cpu, uint8_t bus)
{
    return read(cpu, bus);
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc_clear_ea_high */
static qe6502_tick_t mc_r_zpx_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_r_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));

    return tick;
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static qe6502_tick_t mc_r_zpx_c2_data(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc */
static qe6502_tick_t mc_r_zpy_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=clear_ea_high_dummy_read_zero_page_base_then_add_y_wraparound */
static qe6502_tick_t mc_r_zpy_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->Y));

    return tick;
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static qe6502_tick_t mc_r_zpy_c2_data(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* special_handler; role=dummy; action=dummy_read_pc_before_stack_pull */
static qe6502_tick_t mc_rti_c0_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* special_handler; role=prepull; action=dummy_stack_read_before_incrementing_s */
static qe6502_tick_t mc_rti_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* special_handler; role=pull_p; action=increment_s_and_read_status_from_stack */
static qe6502_tick_t mc_rti_c2_pull_p(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_read(cpu);
}

/* special_handler; role=pull_pcl; action=normalize_status_increment_s_and_read_pc_low */
static qe6502_tick_t mc_rti_c3_pull_pcl(qe6502_t* cpu, uint8_t bus)
{
    cpu->P = (uint8_t)((bus | flag_UN) & (uint8_t)(~flag_B));

    return stack_read(cpu);
}

/* special_handler; role=pull_pch; action=increment_s_and_read_pc_high */
static qe6502_tick_t mc_rti_c4_pull_pch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 0, bus);

    return stack_read(cpu);
}

/* special_handler; role=fetch; action=consume_pc_high_and_fetch_next_opcode */
static qe6502_tick_t mc_rti_c5_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 1, bus);

    return mc_fetch(cpu, bus);
}

/* special_handler; role=dummy; action=dummy_read_pc_before_stack_pull */
static qe6502_tick_t mc_rts_c0_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* special_handler; role=prepull; action=dummy_stack_read_before_incrementing_s */
static qe6502_tick_t mc_rts_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* special_handler; role=pull_pcl; action=increment_s_and_read_return_pc_low */
static qe6502_tick_t mc_rts_c2_pull_pcl(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_read(cpu);
}

/* special_handler; role=pull_pch; action=increment_s_and_read_return_pc_high */
static qe6502_tick_t mc_rts_c3_pull_pch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 0, bus);

    return stack_read(cpu);
}

/* special_handler; role=inc_pc_dummy; action=dummy_read_return_address_then_increment_pc */
static qe6502_tick_t mc_rts_c4_inc_pc_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = qe_u16_set_byte(cpu->PC, 1, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_abs c0: request absolute low operand byte */
static inline qe6502_tick_t mc_rw_abs_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_abs c1: capture low byte, request high operand byte */
static inline qe6502_tick_t mc_rw_abs_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_abs c2: capture high byte, read from effective address */
static inline qe6502_tick_t mc_rw_abs_c2_data(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* rw_abs c3: capture old memory value, write it back unchanged */
static inline qe6502_tick_t mc_rw_abs_c3_wb(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* rw_abx c0: request absolute low operand byte */
static inline qe6502_tick_t mc_rw_abx_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_abx c1: capture low byte, add X to low byte, request high operand byte */
static inline qe6502_tick_t mc_rw_abx_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)((uint16_t)bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_abx c2: capture high byte, perform indexed dummy/probe read, then fix high byte */
static inline qe6502_tick_t mc_rw_abx_c2_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    return tick;
}

/* rw_abx c3: ignore probe bus and read from corrected effective address */
static inline qe6502_tick_t mc_rw_abx_c3_data(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* rw_abx c4: capture old memory value and write it back unchanged */
static inline qe6502_tick_t mc_rw_abx_c4_wb(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* rw_zp c0: request zero-page address operand byte */
static inline qe6502_tick_t mc_rw_zp_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_zp c1: capture zero-page address and read old memory value */
static inline qe6502_tick_t mc_rw_zp_c1_data(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* rw_zp c2: capture old memory value and write it back unchanged */
static inline qe6502_tick_t mc_rw_zp_c2_wb(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* rw_zpx c0: request zero-page base address operand byte */
static inline qe6502_tick_t mc_rw_zpx_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* rw_zpx c1: capture base address, dummy-read it, then add X with zero-page wraparound */
static inline qe6502_tick_t mc_rw_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));

    return tick;
}

/* rw_zpx c2: ignore dummy bus and read old memory value from indexed zero-page address */
static inline qe6502_tick_t mc_rw_zpx_c2_data(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* rw_zpx c3: capture old memory value and write it back unchanged */
static inline qe6502_tick_t mc_rw_zpx_c3_wb(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* special_handler; role=dummy; action=dummy_read_pc_before_stack_pull */
static qe6502_tick_t mc_stack_pull_c0_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* special_handler; role=prepull; action=dummy_stack_read_before_incrementing_s */
static qe6502_tick_t mc_stack_pull_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* special_handler; role=read; action=increment_s_and_read_stack_value */
static qe6502_tick_t mc_stack_pull_c2_read(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_read(cpu);
}

/* special_handler; role=dummy; action=dummy_read_pc_before_stack_push */
static qe6502_tick_t mc_stack_push_c0_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static qe6502_tick_t mc_w_abs_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_and_increment_pc */
static qe6502_tick_t mc_w_abs_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static qe6502_tick_t mc_w_abx_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_increment_pc_add_x_low_and_record_page_cross */
static qe6502_tick_t mc_w_abx_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=probe; action=dummy_read_indexed_probe_then_fix_ea_high */
static qe6502_tick_t mc_w_abx_c2_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    return tick;
}

/* addressing_handler; role=lo; action=read_pc_to_ea_low_and_increment_pc */
static qe6502_tick_t mc_w_aby_c0_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=hi; action=read_pc_to_ea_high_increment_pc_add_y_low_and_record_page_cross */
static qe6502_tick_t mc_w_aby_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=probe; action=dummy_read_indexed_probe_then_fix_ea_high */
static qe6502_tick_t mc_w_aby_c2_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    return tick;
}

/* addressing_handler; role=ptr; action=read_pc_to_zp_pointer_and_increment_pc */
static qe6502_tick_t mc_w_izx_c0_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zp_pointer_then_add_x_wraparound */
static qe6502_tick_t mc_w_izx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;

    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->X);

    return tick;
}

/* addressing_handler; role=ptrlo; action=read_zp_pointer_to_ea_low_then_increment_pointer_wraparound */
static qe6502_tick_t mc_w_izx_c2_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t addr = cpu->latch_addr;
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(qe_u16_byte(addr, 0) + 1u));
    return tick;
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high */
static qe6502_tick_t mc_w_izx_c3_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t ptr_hi_addr = cpu->latch_addr;

    set_latch_addr0(cpu, bus);
    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=ptr; action=read_pc_to_zp_pointer_and_increment_pc */
static qe6502_tick_t mc_w_izy_c0_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=ptrlo; action=read_zp_pointer_to_ea_low_then_increment_pointer_wraparound */
static qe6502_tick_t mc_w_izy_c1_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high_add_y_low_and_record_page_cross */
static qe6502_tick_t mc_w_izy_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t ptr = qe_u16_byte(cpu->latch_addr, 0);
    const uint16_t ptr_hi_addr = (uint8_t)(ptr + 1u);
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=probe; action=dummy_read_indexed_probe_then_fix_ea_high */
static qe6502_tick_t mc_w_izy_c3_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    const uint16_t addr = qe_u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    return tick;
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc_clear_ea_high */
static qe6502_tick_t mc_w_zp_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc_clear_ea_high */
static qe6502_tick_t mc_w_zpx_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_w_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;

    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->X);

    return tick;
}

/* addressing_handler; role=addr; action=read_pc_to_ea_low_increment_pc_clear_ea_high */
static qe6502_tick_t mc_w_zpy_c0_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_y_wraparound */
static qe6502_tick_t mc_w_zpy_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;

    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->Y);

    return tick;
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_adc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    /* TODO: implement reader mnemonic semantics before fetching next opcode. */
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_and_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->A = (uint8_t)(cpu->A & bus);
    update_flags_nz(cpu, cpu->A);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_asl_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: implement accumulator mnemonic semantics. */
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_asl_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

static inline qe6502_tick_t branch_c0_offset(qe6502_t* cpu, bool taken)
{
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;

    if (!taken)
    {
        cpu->microcode = (uint16_t)(cpu->microcode + 2u);
    }

    return tick;
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bcc_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_C) == 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bcs_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_C) != 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_beq_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_Z) != 0u);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_bit_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t mask = (uint8_t)(flag_Z | flag_V | flag_N);
    const uint8_t flags = (uint8_t)(
        flag_Z_if((cpu->A & bus) == 0u) |
        (bus & (uint8_t)(flag_V | flag_N))
        );

    cpu->latch_data = bus;
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bmi_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_N) != 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bne_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_Z) == 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bpl_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_N) == 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bvc_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_V) == 0u);
}

/* mnemonic_handler; role=offset; action=read_branch_offset_and_select_not_taken_or_taken_path */
static qe6502_tick_t op_bvs_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, (cpu->P & flag_V) != 0u);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_clc_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_C));
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_cld_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_D));
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_cli_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_I));
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_clv_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_V));
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cmp_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    compare_register_with_value(cpu, cpu->A, bus);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cpx_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    compare_register_with_value(cpu, cpu->X, bus);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_cpy_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    compare_register_with_value(cpu, cpu->Y, bus);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_dec_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_dex_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->X--;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_dey_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->Y--;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_eor_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->A = (uint8_t)(cpu->A ^ bus);
    update_flags_nz(cpu, cpu->A);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_inc_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_inx_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->X++;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_iny_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->Y++;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_lda_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->A = bus;
    update_flags_nz(cpu, cpu->A);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_ldx_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->X = bus;
    update_flags_nz(cpu, cpu->X);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_ldy_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->Y = bus;
    update_flags_nz(cpu, cpu->Y);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_lsr_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: implement accumulator mnemonic semantics. */
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_lsr_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_nop_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_ora_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    cpu->A = (uint8_t)(cpu->A | bus);
    update_flags_nz(cpu, cpu->A);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=write; action=write_mnemonic_value_to_stack_and_decrement_s */
static qe6502_tick_t op_pha_stack_push_ready_none_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, cpu->A);
}

/* mnemonic_handler; role=write; action=write_mnemonic_value_to_stack_and_decrement_s */
static qe6502_tick_t op_php_stack_push_ready_none_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, (uint8_t)(cpu->P | flag_B));
}

/* mnemonic_handler; role=exec_fetch; action=consume_pulled_stack_value_apply_mnemonic_semantics_and_fetch_next_opcode */
static qe6502_tick_t op_pla_stack_pull_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->A = bus;
    update_flags_nz(cpu, cpu->A);

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_fetch; action=consume_pulled_stack_value_apply_mnemonic_semantics_and_fetch_next_opcode */
static qe6502_tick_t op_plp_stack_pull_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->P = (uint8_t)((bus | flag_UN) & (uint8_t)(~flag_B));

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_rol_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: implement accumulator mnemonic semantics. */
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_rol_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_ror_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: implement accumulator mnemonic semantics. */
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_ror_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    /* TODO: modify cpu->latch_data and update flags before final write. */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_sbc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    /* TODO: implement reader mnemonic semantics before fetching next opcode. */
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_sec_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P | flag_C);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_sed_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P | flag_D);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_sei_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->P = (uint8_t)(cpu->P | flag_I);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sta_w_ready_addr_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return write(cpu, cpu->latch_addr, cpu->A);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sta_w_ready_addrlo_pending_addrhi_wr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return write(cpu, cpu->latch_addr, cpu->A);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sta_w_ready_none_pending_addrlo_wr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    return write(cpu, cpu->latch_addr, cpu->A);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_stx_w_ready_addr_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return write(cpu, cpu->latch_addr, cpu->X);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_stx_w_ready_addrlo_pending_addrhi_wr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return write(cpu, cpu->latch_addr, cpu->X);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_stx_w_ready_none_pending_addrlo_wr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    return write(cpu, cpu->latch_addr, cpu->X);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sty_w_ready_addr_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return write(cpu, cpu->latch_addr, cpu->Y);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sty_w_ready_addrlo_pending_addrhi_wr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return write(cpu, cpu->latch_addr, cpu->Y);
}

/* mnemonic_handler; role=exec_write; action=execute_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_sty_w_ready_none_pending_addrlo_wr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    return write(cpu, cpu->latch_addr, cpu->Y);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_tax_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->X = cpu->A;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_tay_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->Y = cpu->A;
    update_flags_nz(cpu, cpu->Y);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_tsx_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->X = cpu->S;
    update_flags_nz(cpu, cpu->X);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_txa_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A = cpu->X;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_txs_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->S = cpu->X;
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_dummy; action=execute_implied_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_tya_imp_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A = cpu->Y;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

#define IDX(opcode, cycle) ((((opcode) & 0x1FFu) << 3u) + (cycle))

const qe6502_microcode_fn qe6502_microcode_table[2176] =
    {
        /* 0x00 BRK ; class=brk */
        [IDX(0x00, 0)] = &mc_brk_c0_pad,
        [IDX(0x00, 1)] = &mc_brk_c1_push_pch,
        [IDX(0x00, 2)] = &mc_brk_c2_push_pcl,
        [IDX(0x00, 3)] = &mc_brk_c3_push_p,
        [IDX(0x00, 4)] = &mc_brk_c4_vec_lo,
        [IDX(0x00, 5)] = &mc_brk_c5_vec_hi,
        [IDX(0x00, 6)] = &mc_fetch_no_interrupts,
        [IDX(0x00, 7)] = &mc_dispatch,

        /* 0x01 ORA (zp,X) ; class=r_izx */
        [IDX(0x01, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0x01, 1)] = &mc_r_izx_c1_idx,
        [IDX(0x01, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0x01, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0x01, 4)] = &mc_r_izx_c4_data,
        [IDX(0x01, 5)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x01, 6)] = &mc_dispatch,
        [IDX(0x01, 7)] = &op_ILL,

        /* 0x02 undocumented/illegal */
        [IDX(0x02, 0)] = &op_ILL,
        [IDX(0x02, 1)] = &op_ILL,
        [IDX(0x02, 2)] = &op_ILL,
        [IDX(0x02, 3)] = &op_ILL,
        [IDX(0x02, 4)] = &op_ILL,
        [IDX(0x02, 5)] = &op_ILL,
        [IDX(0x02, 6)] = &op_ILL,
        [IDX(0x02, 7)] = &op_ILL,

        /* 0x03 undocumented/illegal */
        [IDX(0x03, 0)] = &op_ILL,
        [IDX(0x03, 1)] = &op_ILL,
        [IDX(0x03, 2)] = &op_ILL,
        [IDX(0x03, 3)] = &op_ILL,
        [IDX(0x03, 4)] = &op_ILL,
        [IDX(0x03, 5)] = &op_ILL,
        [IDX(0x03, 6)] = &op_ILL,
        [IDX(0x03, 7)] = &op_ILL,

        /* 0x04 undocumented/illegal */
        [IDX(0x04, 0)] = &op_ILL,
        [IDX(0x04, 1)] = &op_ILL,
        [IDX(0x04, 2)] = &op_ILL,
        [IDX(0x04, 3)] = &op_ILL,
        [IDX(0x04, 4)] = &op_ILL,
        [IDX(0x04, 5)] = &op_ILL,
        [IDX(0x04, 6)] = &op_ILL,
        [IDX(0x04, 7)] = &op_ILL,

        /* 0x05 ORA zp ; class=r_zp */
        [IDX(0x05, 0)] = &mc_r_zp_c0_addr,
        [IDX(0x05, 1)] = &mc_r_zp_c1_data,
        [IDX(0x05, 2)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x05, 3)] = &mc_dispatch,
        [IDX(0x05, 4)] = &op_ILL,
        [IDX(0x05, 5)] = &op_ILL,
        [IDX(0x05, 6)] = &op_ILL,
        [IDX(0x05, 7)] = &op_ILL,

        /* 0x06 ASL zp ; class=rw_zp */
        [IDX(0x06, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0x06, 1)] = &mc_rw_zp_c1_data,
        [IDX(0x06, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0x06, 3)] = &op_asl_rw_ready_addr_data_pending_none_wr,
        [IDX(0x06, 4)] = &mc_fetch,
        [IDX(0x06, 5)] = &mc_dispatch,
        [IDX(0x06, 6)] = &op_ILL,
        [IDX(0x06, 7)] = &op_ILL,

        /* 0x07 undocumented/illegal */
        [IDX(0x07, 0)] = &op_ILL,
        [IDX(0x07, 1)] = &op_ILL,
        [IDX(0x07, 2)] = &op_ILL,
        [IDX(0x07, 3)] = &op_ILL,
        [IDX(0x07, 4)] = &op_ILL,
        [IDX(0x07, 5)] = &op_ILL,
        [IDX(0x07, 6)] = &op_ILL,
        [IDX(0x07, 7)] = &op_ILL,

        /* 0x08 PHP ; class=stack_push */
        [IDX(0x08, 0)] = &mc_stack_push_c0_dummy,
        [IDX(0x08, 1)] = &op_php_stack_push_ready_none_pending_none_wr,
        [IDX(0x08, 2)] = &mc_fetch,
        [IDX(0x08, 3)] = &mc_dispatch,
        [IDX(0x08, 4)] = &op_ILL,
        [IDX(0x08, 5)] = &op_ILL,
        [IDX(0x08, 6)] = &op_ILL,
        [IDX(0x08, 7)] = &op_ILL,

        /* 0x09 ORA #imm ; class=r_imm */
        [IDX(0x09, 0)] = &mc_r_imm_c0_data,
        [IDX(0x09, 1)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x09, 2)] = &mc_dispatch,
        [IDX(0x09, 3)] = &op_ILL,
        [IDX(0x09, 4)] = &op_ILL,
        [IDX(0x09, 5)] = &op_ILL,
        [IDX(0x09, 6)] = &op_ILL,
        [IDX(0x09, 7)] = &op_ILL,

        /* 0x0A ASL A ; class=acc */
        [IDX(0x0A, 0)] = &op_asl_acc_ready_none_pending_none_dummy,
        [IDX(0x0A, 1)] = &mc_fetch,
        [IDX(0x0A, 2)] = &mc_dispatch,
        [IDX(0x0A, 3)] = &op_ILL,
        [IDX(0x0A, 4)] = &op_ILL,
        [IDX(0x0A, 5)] = &op_ILL,
        [IDX(0x0A, 6)] = &op_ILL,
        [IDX(0x0A, 7)] = &op_ILL,

        /* 0x0B undocumented/illegal */
        [IDX(0x0B, 0)] = &op_ILL,
        [IDX(0x0B, 1)] = &op_ILL,
        [IDX(0x0B, 2)] = &op_ILL,
        [IDX(0x0B, 3)] = &op_ILL,
        [IDX(0x0B, 4)] = &op_ILL,
        [IDX(0x0B, 5)] = &op_ILL,
        [IDX(0x0B, 6)] = &op_ILL,
        [IDX(0x0B, 7)] = &op_ILL,

        /* 0x0C undocumented/illegal */
        [IDX(0x0C, 0)] = &op_ILL,
        [IDX(0x0C, 1)] = &op_ILL,
        [IDX(0x0C, 2)] = &op_ILL,
        [IDX(0x0C, 3)] = &op_ILL,
        [IDX(0x0C, 4)] = &op_ILL,
        [IDX(0x0C, 5)] = &op_ILL,
        [IDX(0x0C, 6)] = &op_ILL,
        [IDX(0x0C, 7)] = &op_ILL,

        /* 0x0D ORA abs ; class=r_abs */
        [IDX(0x0D, 0)] = &mc_r_abs_c0_lo,
        [IDX(0x0D, 1)] = &mc_r_abs_c1_hi,
        [IDX(0x0D, 2)] = &mc_r_abs_c2_data,
        [IDX(0x0D, 3)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x0D, 4)] = &mc_dispatch,
        [IDX(0x0D, 5)] = &op_ILL,
        [IDX(0x0D, 6)] = &op_ILL,
        [IDX(0x0D, 7)] = &op_ILL,

        /* 0x0E ASL abs ; class=rw_abs */
        [IDX(0x0E, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0x0E, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0x0E, 2)] = &mc_rw_abs_c2_data,
        [IDX(0x0E, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0x0E, 4)] = &op_asl_rw_ready_addr_data_pending_none_wr,
        [IDX(0x0E, 5)] = &mc_fetch,
        [IDX(0x0E, 6)] = &mc_dispatch,
        [IDX(0x0E, 7)] = &op_ILL,

        /* 0x0F undocumented/illegal */
        [IDX(0x0F, 0)] = &op_ILL,
        [IDX(0x0F, 1)] = &op_ILL,
        [IDX(0x0F, 2)] = &op_ILL,
        [IDX(0x0F, 3)] = &op_ILL,
        [IDX(0x0F, 4)] = &op_ILL,
        [IDX(0x0F, 5)] = &op_ILL,
        [IDX(0x0F, 6)] = &op_ILL,
        [IDX(0x0F, 7)] = &op_ILL,

        /* 0x10 BPL rel ; class=branch_rel */
        [IDX(0x10, 0)] = &op_bpl_branch_c0_offset,
        [IDX(0x10, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0x10, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0x10, 3)] = &mc_fetch,
        [IDX(0x10, 4)] = &mc_dispatch,
        [IDX(0x10, 5)] = &op_ILL,
        [IDX(0x10, 6)] = &op_ILL,
        [IDX(0x10, 7)] = &op_ILL,

        /* 0x11 ORA (zp),Y ; class=r_izy */
        [IDX(0x11, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0x11, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0x11, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0x11, 3)] = &mc_r_izy_c3_probe,
        [IDX(0x11, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0x11, 5)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x11, 6)] = &mc_dispatch,
        [IDX(0x11, 7)] = &op_ILL,

        /* 0x12 undocumented/illegal */
        [IDX(0x12, 0)] = &op_ILL,
        [IDX(0x12, 1)] = &op_ILL,
        [IDX(0x12, 2)] = &op_ILL,
        [IDX(0x12, 3)] = &op_ILL,
        [IDX(0x12, 4)] = &op_ILL,
        [IDX(0x12, 5)] = &op_ILL,
        [IDX(0x12, 6)] = &op_ILL,
        [IDX(0x12, 7)] = &op_ILL,

        /* 0x13 undocumented/illegal */
        [IDX(0x13, 0)] = &op_ILL,
        [IDX(0x13, 1)] = &op_ILL,
        [IDX(0x13, 2)] = &op_ILL,
        [IDX(0x13, 3)] = &op_ILL,
        [IDX(0x13, 4)] = &op_ILL,
        [IDX(0x13, 5)] = &op_ILL,
        [IDX(0x13, 6)] = &op_ILL,
        [IDX(0x13, 7)] = &op_ILL,

        /* 0x14 undocumented/illegal */
        [IDX(0x14, 0)] = &op_ILL,
        [IDX(0x14, 1)] = &op_ILL,
        [IDX(0x14, 2)] = &op_ILL,
        [IDX(0x14, 3)] = &op_ILL,
        [IDX(0x14, 4)] = &op_ILL,
        [IDX(0x14, 5)] = &op_ILL,
        [IDX(0x14, 6)] = &op_ILL,
        [IDX(0x14, 7)] = &op_ILL,

        /* 0x15 ORA zp,X ; class=r_zpx */
        [IDX(0x15, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0x15, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0x15, 2)] = &mc_r_zpx_c2_data,
        [IDX(0x15, 3)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x15, 4)] = &mc_dispatch,
        [IDX(0x15, 5)] = &op_ILL,
        [IDX(0x15, 6)] = &op_ILL,
        [IDX(0x15, 7)] = &op_ILL,

        /* 0x16 ASL zp,X ; class=rw_zpx */
        [IDX(0x16, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0x16, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0x16, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0x16, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0x16, 4)] = &op_asl_rw_ready_addr_data_pending_none_wr,
        [IDX(0x16, 5)] = &mc_fetch,
        [IDX(0x16, 6)] = &mc_dispatch,
        [IDX(0x16, 7)] = &op_ILL,

        /* 0x17 undocumented/illegal */
        [IDX(0x17, 0)] = &op_ILL,
        [IDX(0x17, 1)] = &op_ILL,
        [IDX(0x17, 2)] = &op_ILL,
        [IDX(0x17, 3)] = &op_ILL,
        [IDX(0x17, 4)] = &op_ILL,
        [IDX(0x17, 5)] = &op_ILL,
        [IDX(0x17, 6)] = &op_ILL,
        [IDX(0x17, 7)] = &op_ILL,

        /* 0x18 CLC ; class=imp */
        [IDX(0x18, 0)] = &op_clc_imp_ready_none_pending_none_dummy,
        [IDX(0x18, 1)] = &mc_fetch,
        [IDX(0x18, 2)] = &mc_dispatch,
        [IDX(0x18, 3)] = &op_ILL,
        [IDX(0x18, 4)] = &op_ILL,
        [IDX(0x18, 5)] = &op_ILL,
        [IDX(0x18, 6)] = &op_ILL,
        [IDX(0x18, 7)] = &op_ILL,

        /* 0x19 ORA abs,Y ; class=r_aby */
        [IDX(0x19, 0)] = &mc_r_aby_c0_lo,
        [IDX(0x19, 1)] = &mc_r_aby_c1_hi,
        [IDX(0x19, 2)] = &mc_r_aby_c2_probe,
        [IDX(0x19, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0x19, 4)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x19, 5)] = &mc_dispatch,
        [IDX(0x19, 6)] = &op_ILL,
        [IDX(0x19, 7)] = &op_ILL,

        /* 0x1A undocumented/illegal */
        [IDX(0x1A, 0)] = &op_ILL,
        [IDX(0x1A, 1)] = &op_ILL,
        [IDX(0x1A, 2)] = &op_ILL,
        [IDX(0x1A, 3)] = &op_ILL,
        [IDX(0x1A, 4)] = &op_ILL,
        [IDX(0x1A, 5)] = &op_ILL,
        [IDX(0x1A, 6)] = &op_ILL,
        [IDX(0x1A, 7)] = &op_ILL,

        /* 0x1B undocumented/illegal */
        [IDX(0x1B, 0)] = &op_ILL,
        [IDX(0x1B, 1)] = &op_ILL,
        [IDX(0x1B, 2)] = &op_ILL,
        [IDX(0x1B, 3)] = &op_ILL,
        [IDX(0x1B, 4)] = &op_ILL,
        [IDX(0x1B, 5)] = &op_ILL,
        [IDX(0x1B, 6)] = &op_ILL,
        [IDX(0x1B, 7)] = &op_ILL,

        /* 0x1C undocumented/illegal */
        [IDX(0x1C, 0)] = &op_ILL,
        [IDX(0x1C, 1)] = &op_ILL,
        [IDX(0x1C, 2)] = &op_ILL,
        [IDX(0x1C, 3)] = &op_ILL,
        [IDX(0x1C, 4)] = &op_ILL,
        [IDX(0x1C, 5)] = &op_ILL,
        [IDX(0x1C, 6)] = &op_ILL,
        [IDX(0x1C, 7)] = &op_ILL,

        /* 0x1D ORA abs,X ; class=r_abx */
        [IDX(0x1D, 0)] = &mc_r_abx_c0_lo,
        [IDX(0x1D, 1)] = &mc_r_abx_c1_hi,
        [IDX(0x1D, 2)] = &mc_r_abx_c2_probe,
        [IDX(0x1D, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0x1D, 4)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(0x1D, 5)] = &mc_dispatch,
        [IDX(0x1D, 6)] = &op_ILL,
        [IDX(0x1D, 7)] = &op_ILL,

        /* 0x1E ASL abs,X ; class=rw_abx */
        [IDX(0x1E, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0x1E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0x1E, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0x1E, 3)] = &mc_rw_abx_c3_data,
        [IDX(0x1E, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0x1E, 5)] = &op_asl_rw_ready_addr_data_pending_none_wr,
        [IDX(0x1E, 6)] = &mc_fetch,
        [IDX(0x1E, 7)] = &mc_dispatch,

        /* 0x1F undocumented/illegal */
        [IDX(0x1F, 0)] = &op_ILL,
        [IDX(0x1F, 1)] = &op_ILL,
        [IDX(0x1F, 2)] = &op_ILL,
        [IDX(0x1F, 3)] = &op_ILL,
        [IDX(0x1F, 4)] = &op_ILL,
        [IDX(0x1F, 5)] = &op_ILL,
        [IDX(0x1F, 6)] = &op_ILL,
        [IDX(0x1F, 7)] = &op_ILL,

        /* 0x20 JSR abs ; class=jsr_abs */
        [IDX(0x20, 0)] = &mc_jsr_abs_c0_lo,
        [IDX(0x20, 1)] = &mc_jsr_abs_c1_dummy,
        [IDX(0x20, 2)] = &mc_jsr_abs_c2_push_pch,
        [IDX(0x20, 3)] = &mc_jsr_abs_c3_push_pcl,
        [IDX(0x20, 4)] = &mc_jsr_abs_c4_hi,
        [IDX(0x20, 5)] = &mc_jsr_abs_c5_fetch,
        [IDX(0x20, 6)] = &mc_dispatch,
        [IDX(0x20, 7)] = &op_ILL,

        /* 0x21 AND (zp,X) ; class=r_izx */
        [IDX(0x21, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0x21, 1)] = &mc_r_izx_c1_idx,
        [IDX(0x21, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0x21, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0x21, 4)] = &mc_r_izx_c4_data,
        [IDX(0x21, 5)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x21, 6)] = &mc_dispatch,
        [IDX(0x21, 7)] = &op_ILL,

        /* 0x22 undocumented/illegal */
        [IDX(0x22, 0)] = &op_ILL,
        [IDX(0x22, 1)] = &op_ILL,
        [IDX(0x22, 2)] = &op_ILL,
        [IDX(0x22, 3)] = &op_ILL,
        [IDX(0x22, 4)] = &op_ILL,
        [IDX(0x22, 5)] = &op_ILL,
        [IDX(0x22, 6)] = &op_ILL,
        [IDX(0x22, 7)] = &op_ILL,

        /* 0x23 undocumented/illegal */
        [IDX(0x23, 0)] = &op_ILL,
        [IDX(0x23, 1)] = &op_ILL,
        [IDX(0x23, 2)] = &op_ILL,
        [IDX(0x23, 3)] = &op_ILL,
        [IDX(0x23, 4)] = &op_ILL,
        [IDX(0x23, 5)] = &op_ILL,
        [IDX(0x23, 6)] = &op_ILL,
        [IDX(0x23, 7)] = &op_ILL,

        /* 0x24 BIT zp ; class=r_zp */
        [IDX(0x24, 0)] = &mc_r_zp_c0_addr,
        [IDX(0x24, 1)] = &mc_r_zp_c1_data,
        [IDX(0x24, 2)] = &op_bit_r_ready_none_pending_data_fetch,
        [IDX(0x24, 3)] = &mc_dispatch,
        [IDX(0x24, 4)] = &op_ILL,
        [IDX(0x24, 5)] = &op_ILL,
        [IDX(0x24, 6)] = &op_ILL,
        [IDX(0x24, 7)] = &op_ILL,

        /* 0x25 AND zp ; class=r_zp */
        [IDX(0x25, 0)] = &mc_r_zp_c0_addr,
        [IDX(0x25, 1)] = &mc_r_zp_c1_data,
        [IDX(0x25, 2)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x25, 3)] = &mc_dispatch,
        [IDX(0x25, 4)] = &op_ILL,
        [IDX(0x25, 5)] = &op_ILL,
        [IDX(0x25, 6)] = &op_ILL,
        [IDX(0x25, 7)] = &op_ILL,

        /* 0x26 ROL zp ; class=rw_zp */
        [IDX(0x26, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0x26, 1)] = &mc_rw_zp_c1_data,
        [IDX(0x26, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0x26, 3)] = &op_rol_rw_ready_addr_data_pending_none_wr,
        [IDX(0x26, 4)] = &mc_fetch,
        [IDX(0x26, 5)] = &mc_dispatch,
        [IDX(0x26, 6)] = &op_ILL,
        [IDX(0x26, 7)] = &op_ILL,

        /* 0x27 undocumented/illegal */
        [IDX(0x27, 0)] = &op_ILL,
        [IDX(0x27, 1)] = &op_ILL,
        [IDX(0x27, 2)] = &op_ILL,
        [IDX(0x27, 3)] = &op_ILL,
        [IDX(0x27, 4)] = &op_ILL,
        [IDX(0x27, 5)] = &op_ILL,
        [IDX(0x27, 6)] = &op_ILL,
        [IDX(0x27, 7)] = &op_ILL,

        /* 0x28 PLP ; class=stack_pull */
        [IDX(0x28, 0)] = &mc_stack_pull_c0_dummy,
        [IDX(0x28, 1)] = &mc_stack_pull_c1_dummy,
        [IDX(0x28, 2)] = &mc_stack_pull_c2_read,
        [IDX(0x28, 3)] = &op_plp_stack_pull_ready_none_pending_data_fetch,
        [IDX(0x28, 4)] = &mc_dispatch,
        [IDX(0x28, 5)] = &op_ILL,
        [IDX(0x28, 6)] = &op_ILL,
        [IDX(0x28, 7)] = &op_ILL,

        /* 0x29 AND #imm ; class=r_imm */
        [IDX(0x29, 0)] = &mc_r_imm_c0_data,
        [IDX(0x29, 1)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x29, 2)] = &mc_dispatch,
        [IDX(0x29, 3)] = &op_ILL,
        [IDX(0x29, 4)] = &op_ILL,
        [IDX(0x29, 5)] = &op_ILL,
        [IDX(0x29, 6)] = &op_ILL,
        [IDX(0x29, 7)] = &op_ILL,

        /* 0x2A ROL A ; class=acc */
        [IDX(0x2A, 0)] = &op_rol_acc_ready_none_pending_none_dummy,
        [IDX(0x2A, 1)] = &mc_fetch,
        [IDX(0x2A, 2)] = &mc_dispatch,
        [IDX(0x2A, 3)] = &op_ILL,
        [IDX(0x2A, 4)] = &op_ILL,
        [IDX(0x2A, 5)] = &op_ILL,
        [IDX(0x2A, 6)] = &op_ILL,
        [IDX(0x2A, 7)] = &op_ILL,

        /* 0x2B undocumented/illegal */
        [IDX(0x2B, 0)] = &op_ILL,
        [IDX(0x2B, 1)] = &op_ILL,
        [IDX(0x2B, 2)] = &op_ILL,
        [IDX(0x2B, 3)] = &op_ILL,
        [IDX(0x2B, 4)] = &op_ILL,
        [IDX(0x2B, 5)] = &op_ILL,
        [IDX(0x2B, 6)] = &op_ILL,
        [IDX(0x2B, 7)] = &op_ILL,

        /* 0x2C BIT abs ; class=r_abs */
        [IDX(0x2C, 0)] = &mc_r_abs_c0_lo,
        [IDX(0x2C, 1)] = &mc_r_abs_c1_hi,
        [IDX(0x2C, 2)] = &mc_r_abs_c2_data,
        [IDX(0x2C, 3)] = &op_bit_r_ready_none_pending_data_fetch,
        [IDX(0x2C, 4)] = &mc_dispatch,
        [IDX(0x2C, 5)] = &op_ILL,
        [IDX(0x2C, 6)] = &op_ILL,
        [IDX(0x2C, 7)] = &op_ILL,

        /* 0x2D AND abs ; class=r_abs */
        [IDX(0x2D, 0)] = &mc_r_abs_c0_lo,
        [IDX(0x2D, 1)] = &mc_r_abs_c1_hi,
        [IDX(0x2D, 2)] = &mc_r_abs_c2_data,
        [IDX(0x2D, 3)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x2D, 4)] = &mc_dispatch,
        [IDX(0x2D, 5)] = &op_ILL,
        [IDX(0x2D, 6)] = &op_ILL,
        [IDX(0x2D, 7)] = &op_ILL,

        /* 0x2E ROL abs ; class=rw_abs */
        [IDX(0x2E, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0x2E, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0x2E, 2)] = &mc_rw_abs_c2_data,
        [IDX(0x2E, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0x2E, 4)] = &op_rol_rw_ready_addr_data_pending_none_wr,
        [IDX(0x2E, 5)] = &mc_fetch,
        [IDX(0x2E, 6)] = &mc_dispatch,
        [IDX(0x2E, 7)] = &op_ILL,

        /* 0x2F undocumented/illegal */
        [IDX(0x2F, 0)] = &op_ILL,
        [IDX(0x2F, 1)] = &op_ILL,
        [IDX(0x2F, 2)] = &op_ILL,
        [IDX(0x2F, 3)] = &op_ILL,
        [IDX(0x2F, 4)] = &op_ILL,
        [IDX(0x2F, 5)] = &op_ILL,
        [IDX(0x2F, 6)] = &op_ILL,
        [IDX(0x2F, 7)] = &op_ILL,

        /* 0x30 BMI rel ; class=branch_rel */
        [IDX(0x30, 0)] = &op_bmi_branch_c0_offset,
        [IDX(0x30, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0x30, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0x30, 3)] = &mc_fetch,
        [IDX(0x30, 4)] = &mc_dispatch,
        [IDX(0x30, 5)] = &op_ILL,
        [IDX(0x30, 6)] = &op_ILL,
        [IDX(0x30, 7)] = &op_ILL,

        /* 0x31 AND (zp),Y ; class=r_izy */
        [IDX(0x31, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0x31, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0x31, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0x31, 3)] = &mc_r_izy_c3_probe,
        [IDX(0x31, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0x31, 5)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x31, 6)] = &mc_dispatch,
        [IDX(0x31, 7)] = &op_ILL,

        /* 0x32 undocumented/illegal */
        [IDX(0x32, 0)] = &op_ILL,
        [IDX(0x32, 1)] = &op_ILL,
        [IDX(0x32, 2)] = &op_ILL,
        [IDX(0x32, 3)] = &op_ILL,
        [IDX(0x32, 4)] = &op_ILL,
        [IDX(0x32, 5)] = &op_ILL,
        [IDX(0x32, 6)] = &op_ILL,
        [IDX(0x32, 7)] = &op_ILL,

        /* 0x33 undocumented/illegal */
        [IDX(0x33, 0)] = &op_ILL,
        [IDX(0x33, 1)] = &op_ILL,
        [IDX(0x33, 2)] = &op_ILL,
        [IDX(0x33, 3)] = &op_ILL,
        [IDX(0x33, 4)] = &op_ILL,
        [IDX(0x33, 5)] = &op_ILL,
        [IDX(0x33, 6)] = &op_ILL,
        [IDX(0x33, 7)] = &op_ILL,

        /* 0x34 undocumented/illegal */
        [IDX(0x34, 0)] = &op_ILL,
        [IDX(0x34, 1)] = &op_ILL,
        [IDX(0x34, 2)] = &op_ILL,
        [IDX(0x34, 3)] = &op_ILL,
        [IDX(0x34, 4)] = &op_ILL,
        [IDX(0x34, 5)] = &op_ILL,
        [IDX(0x34, 6)] = &op_ILL,
        [IDX(0x34, 7)] = &op_ILL,

        /* 0x35 AND zp,X ; class=r_zpx */
        [IDX(0x35, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0x35, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0x35, 2)] = &mc_r_zpx_c2_data,
        [IDX(0x35, 3)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x35, 4)] = &mc_dispatch,
        [IDX(0x35, 5)] = &op_ILL,
        [IDX(0x35, 6)] = &op_ILL,
        [IDX(0x35, 7)] = &op_ILL,

        /* 0x36 ROL zp,X ; class=rw_zpx */
        [IDX(0x36, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0x36, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0x36, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0x36, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0x36, 4)] = &op_rol_rw_ready_addr_data_pending_none_wr,
        [IDX(0x36, 5)] = &mc_fetch,
        [IDX(0x36, 6)] = &mc_dispatch,
        [IDX(0x36, 7)] = &op_ILL,

        /* 0x37 undocumented/illegal */
        [IDX(0x37, 0)] = &op_ILL,
        [IDX(0x37, 1)] = &op_ILL,
        [IDX(0x37, 2)] = &op_ILL,
        [IDX(0x37, 3)] = &op_ILL,
        [IDX(0x37, 4)] = &op_ILL,
        [IDX(0x37, 5)] = &op_ILL,
        [IDX(0x37, 6)] = &op_ILL,
        [IDX(0x37, 7)] = &op_ILL,

        /* 0x38 SEC ; class=imp */
        [IDX(0x38, 0)] = &op_sec_imp_ready_none_pending_none_dummy,
        [IDX(0x38, 1)] = &mc_fetch,
        [IDX(0x38, 2)] = &mc_dispatch,
        [IDX(0x38, 3)] = &op_ILL,
        [IDX(0x38, 4)] = &op_ILL,
        [IDX(0x38, 5)] = &op_ILL,
        [IDX(0x38, 6)] = &op_ILL,
        [IDX(0x38, 7)] = &op_ILL,

        /* 0x39 AND abs,Y ; class=r_aby */
        [IDX(0x39, 0)] = &mc_r_aby_c0_lo,
        [IDX(0x39, 1)] = &mc_r_aby_c1_hi,
        [IDX(0x39, 2)] = &mc_r_aby_c2_probe,
        [IDX(0x39, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0x39, 4)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x39, 5)] = &mc_dispatch,
        [IDX(0x39, 6)] = &op_ILL,
        [IDX(0x39, 7)] = &op_ILL,

        /* 0x3A undocumented/illegal */
        [IDX(0x3A, 0)] = &op_ILL,
        [IDX(0x3A, 1)] = &op_ILL,
        [IDX(0x3A, 2)] = &op_ILL,
        [IDX(0x3A, 3)] = &op_ILL,
        [IDX(0x3A, 4)] = &op_ILL,
        [IDX(0x3A, 5)] = &op_ILL,
        [IDX(0x3A, 6)] = &op_ILL,
        [IDX(0x3A, 7)] = &op_ILL,

        /* 0x3B undocumented/illegal */
        [IDX(0x3B, 0)] = &op_ILL,
        [IDX(0x3B, 1)] = &op_ILL,
        [IDX(0x3B, 2)] = &op_ILL,
        [IDX(0x3B, 3)] = &op_ILL,
        [IDX(0x3B, 4)] = &op_ILL,
        [IDX(0x3B, 5)] = &op_ILL,
        [IDX(0x3B, 6)] = &op_ILL,
        [IDX(0x3B, 7)] = &op_ILL,

        /* 0x3C undocumented/illegal */
        [IDX(0x3C, 0)] = &op_ILL,
        [IDX(0x3C, 1)] = &op_ILL,
        [IDX(0x3C, 2)] = &op_ILL,
        [IDX(0x3C, 3)] = &op_ILL,
        [IDX(0x3C, 4)] = &op_ILL,
        [IDX(0x3C, 5)] = &op_ILL,
        [IDX(0x3C, 6)] = &op_ILL,
        [IDX(0x3C, 7)] = &op_ILL,

        /* 0x3D AND abs,X ; class=r_abx */
        [IDX(0x3D, 0)] = &mc_r_abx_c0_lo,
        [IDX(0x3D, 1)] = &mc_r_abx_c1_hi,
        [IDX(0x3D, 2)] = &mc_r_abx_c2_probe,
        [IDX(0x3D, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0x3D, 4)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(0x3D, 5)] = &mc_dispatch,
        [IDX(0x3D, 6)] = &op_ILL,
        [IDX(0x3D, 7)] = &op_ILL,

        /* 0x3E ROL abs,X ; class=rw_abx */
        [IDX(0x3E, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0x3E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0x3E, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0x3E, 3)] = &mc_rw_abx_c3_data,
        [IDX(0x3E, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0x3E, 5)] = &op_rol_rw_ready_addr_data_pending_none_wr,
        [IDX(0x3E, 6)] = &mc_fetch,
        [IDX(0x3E, 7)] = &mc_dispatch,

        /* 0x3F undocumented/illegal */
        [IDX(0x3F, 0)] = &op_ILL,
        [IDX(0x3F, 1)] = &op_ILL,
        [IDX(0x3F, 2)] = &op_ILL,
        [IDX(0x3F, 3)] = &op_ILL,
        [IDX(0x3F, 4)] = &op_ILL,
        [IDX(0x3F, 5)] = &op_ILL,
        [IDX(0x3F, 6)] = &op_ILL,
        [IDX(0x3F, 7)] = &op_ILL,

        /* 0x40 RTI ; class=rti */
        [IDX(0x40, 0)] = &mc_rti_c0_dummy,
        [IDX(0x40, 1)] = &mc_rti_c1_dummy,
        [IDX(0x40, 2)] = &mc_rti_c2_pull_p,
        [IDX(0x40, 3)] = &mc_rti_c3_pull_pcl,
        [IDX(0x40, 4)] = &mc_rti_c4_pull_pch,
        [IDX(0x40, 5)] = &mc_rti_c5_fetch,
        [IDX(0x40, 6)] = &mc_dispatch,
        [IDX(0x40, 7)] = &op_ILL,

        /* 0x41 EOR (zp,X) ; class=r_izx */
        [IDX(0x41, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0x41, 1)] = &mc_r_izx_c1_idx,
        [IDX(0x41, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0x41, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0x41, 4)] = &mc_r_izx_c4_data,
        [IDX(0x41, 5)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x41, 6)] = &mc_dispatch,
        [IDX(0x41, 7)] = &op_ILL,

        /* 0x42 undocumented/illegal */
        [IDX(0x42, 0)] = &op_ILL,
        [IDX(0x42, 1)] = &op_ILL,
        [IDX(0x42, 2)] = &op_ILL,
        [IDX(0x42, 3)] = &op_ILL,
        [IDX(0x42, 4)] = &op_ILL,
        [IDX(0x42, 5)] = &op_ILL,
        [IDX(0x42, 6)] = &op_ILL,
        [IDX(0x42, 7)] = &op_ILL,

        /* 0x43 undocumented/illegal */
        [IDX(0x43, 0)] = &op_ILL,
        [IDX(0x43, 1)] = &op_ILL,
        [IDX(0x43, 2)] = &op_ILL,
        [IDX(0x43, 3)] = &op_ILL,
        [IDX(0x43, 4)] = &op_ILL,
        [IDX(0x43, 5)] = &op_ILL,
        [IDX(0x43, 6)] = &op_ILL,
        [IDX(0x43, 7)] = &op_ILL,

        /* 0x44 undocumented/illegal */
        [IDX(0x44, 0)] = &op_ILL,
        [IDX(0x44, 1)] = &op_ILL,
        [IDX(0x44, 2)] = &op_ILL,
        [IDX(0x44, 3)] = &op_ILL,
        [IDX(0x44, 4)] = &op_ILL,
        [IDX(0x44, 5)] = &op_ILL,
        [IDX(0x44, 6)] = &op_ILL,
        [IDX(0x44, 7)] = &op_ILL,

        /* 0x45 EOR zp ; class=r_zp */
        [IDX(0x45, 0)] = &mc_r_zp_c0_addr,
        [IDX(0x45, 1)] = &mc_r_zp_c1_data,
        [IDX(0x45, 2)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x45, 3)] = &mc_dispatch,
        [IDX(0x45, 4)] = &op_ILL,
        [IDX(0x45, 5)] = &op_ILL,
        [IDX(0x45, 6)] = &op_ILL,
        [IDX(0x45, 7)] = &op_ILL,

        /* 0x46 LSR zp ; class=rw_zp */
        [IDX(0x46, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0x46, 1)] = &mc_rw_zp_c1_data,
        [IDX(0x46, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0x46, 3)] = &op_lsr_rw_ready_addr_data_pending_none_wr,
        [IDX(0x46, 4)] = &mc_fetch,
        [IDX(0x46, 5)] = &mc_dispatch,
        [IDX(0x46, 6)] = &op_ILL,
        [IDX(0x46, 7)] = &op_ILL,

        /* 0x47 undocumented/illegal */
        [IDX(0x47, 0)] = &op_ILL,
        [IDX(0x47, 1)] = &op_ILL,
        [IDX(0x47, 2)] = &op_ILL,
        [IDX(0x47, 3)] = &op_ILL,
        [IDX(0x47, 4)] = &op_ILL,
        [IDX(0x47, 5)] = &op_ILL,
        [IDX(0x47, 6)] = &op_ILL,
        [IDX(0x47, 7)] = &op_ILL,

        /* 0x48 PHA ; class=stack_push */
        [IDX(0x48, 0)] = &mc_stack_push_c0_dummy,
        [IDX(0x48, 1)] = &op_pha_stack_push_ready_none_pending_none_wr,
        [IDX(0x48, 2)] = &mc_fetch,
        [IDX(0x48, 3)] = &mc_dispatch,
        [IDX(0x48, 4)] = &op_ILL,
        [IDX(0x48, 5)] = &op_ILL,
        [IDX(0x48, 6)] = &op_ILL,
        [IDX(0x48, 7)] = &op_ILL,

        /* 0x49 EOR #imm ; class=r_imm */
        [IDX(0x49, 0)] = &mc_r_imm_c0_data,
        [IDX(0x49, 1)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x49, 2)] = &mc_dispatch,
        [IDX(0x49, 3)] = &op_ILL,
        [IDX(0x49, 4)] = &op_ILL,
        [IDX(0x49, 5)] = &op_ILL,
        [IDX(0x49, 6)] = &op_ILL,
        [IDX(0x49, 7)] = &op_ILL,

        /* 0x4A LSR A ; class=acc */
        [IDX(0x4A, 0)] = &op_lsr_acc_ready_none_pending_none_dummy,
        [IDX(0x4A, 1)] = &mc_fetch,
        [IDX(0x4A, 2)] = &mc_dispatch,
        [IDX(0x4A, 3)] = &op_ILL,
        [IDX(0x4A, 4)] = &op_ILL,
        [IDX(0x4A, 5)] = &op_ILL,
        [IDX(0x4A, 6)] = &op_ILL,
        [IDX(0x4A, 7)] = &op_ILL,

        /* 0x4B undocumented/illegal */
        [IDX(0x4B, 0)] = &op_ILL,
        [IDX(0x4B, 1)] = &op_ILL,
        [IDX(0x4B, 2)] = &op_ILL,
        [IDX(0x4B, 3)] = &op_ILL,
        [IDX(0x4B, 4)] = &op_ILL,
        [IDX(0x4B, 5)] = &op_ILL,
        [IDX(0x4B, 6)] = &op_ILL,
        [IDX(0x4B, 7)] = &op_ILL,

        /* 0x4C JMP abs ; class=jmp_abs */
        [IDX(0x4C, 0)] = &mc_jmp_abs_c0_lo,
        [IDX(0x4C, 1)] = &mc_jmp_abs_c1_hi,
        [IDX(0x4C, 2)] = &mc_jmp_abs_c2_fetch,
        [IDX(0x4C, 3)] = &mc_dispatch,
        [IDX(0x4C, 4)] = &op_ILL,
        [IDX(0x4C, 5)] = &op_ILL,
        [IDX(0x4C, 6)] = &op_ILL,
        [IDX(0x4C, 7)] = &op_ILL,

        /* 0x4D EOR abs ; class=r_abs */
        [IDX(0x4D, 0)] = &mc_r_abs_c0_lo,
        [IDX(0x4D, 1)] = &mc_r_abs_c1_hi,
        [IDX(0x4D, 2)] = &mc_r_abs_c2_data,
        [IDX(0x4D, 3)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x4D, 4)] = &mc_dispatch,
        [IDX(0x4D, 5)] = &op_ILL,
        [IDX(0x4D, 6)] = &op_ILL,
        [IDX(0x4D, 7)] = &op_ILL,

        /* 0x4E LSR abs ; class=rw_abs */
        [IDX(0x4E, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0x4E, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0x4E, 2)] = &mc_rw_abs_c2_data,
        [IDX(0x4E, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0x4E, 4)] = &op_lsr_rw_ready_addr_data_pending_none_wr,
        [IDX(0x4E, 5)] = &mc_fetch,
        [IDX(0x4E, 6)] = &mc_dispatch,
        [IDX(0x4E, 7)] = &op_ILL,

        /* 0x4F undocumented/illegal */
        [IDX(0x4F, 0)] = &op_ILL,
        [IDX(0x4F, 1)] = &op_ILL,
        [IDX(0x4F, 2)] = &op_ILL,
        [IDX(0x4F, 3)] = &op_ILL,
        [IDX(0x4F, 4)] = &op_ILL,
        [IDX(0x4F, 5)] = &op_ILL,
        [IDX(0x4F, 6)] = &op_ILL,
        [IDX(0x4F, 7)] = &op_ILL,

        /* 0x50 BVC rel ; class=branch_rel */
        [IDX(0x50, 0)] = &op_bvc_branch_c0_offset,
        [IDX(0x50, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0x50, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0x50, 3)] = &mc_fetch,
        [IDX(0x50, 4)] = &mc_dispatch,
        [IDX(0x50, 5)] = &op_ILL,
        [IDX(0x50, 6)] = &op_ILL,
        [IDX(0x50, 7)] = &op_ILL,

        /* 0x51 EOR (zp),Y ; class=r_izy */
        [IDX(0x51, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0x51, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0x51, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0x51, 3)] = &mc_r_izy_c3_probe,
        [IDX(0x51, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0x51, 5)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x51, 6)] = &mc_dispatch,
        [IDX(0x51, 7)] = &op_ILL,

        /* 0x52 undocumented/illegal */
        [IDX(0x52, 0)] = &op_ILL,
        [IDX(0x52, 1)] = &op_ILL,
        [IDX(0x52, 2)] = &op_ILL,
        [IDX(0x52, 3)] = &op_ILL,
        [IDX(0x52, 4)] = &op_ILL,
        [IDX(0x52, 5)] = &op_ILL,
        [IDX(0x52, 6)] = &op_ILL,
        [IDX(0x52, 7)] = &op_ILL,

        /* 0x53 undocumented/illegal */
        [IDX(0x53, 0)] = &op_ILL,
        [IDX(0x53, 1)] = &op_ILL,
        [IDX(0x53, 2)] = &op_ILL,
        [IDX(0x53, 3)] = &op_ILL,
        [IDX(0x53, 4)] = &op_ILL,
        [IDX(0x53, 5)] = &op_ILL,
        [IDX(0x53, 6)] = &op_ILL,
        [IDX(0x53, 7)] = &op_ILL,

        /* 0x54 undocumented/illegal */
        [IDX(0x54, 0)] = &op_ILL,
        [IDX(0x54, 1)] = &op_ILL,
        [IDX(0x54, 2)] = &op_ILL,
        [IDX(0x54, 3)] = &op_ILL,
        [IDX(0x54, 4)] = &op_ILL,
        [IDX(0x54, 5)] = &op_ILL,
        [IDX(0x54, 6)] = &op_ILL,
        [IDX(0x54, 7)] = &op_ILL,

        /* 0x55 EOR zp,X ; class=r_zpx */
        [IDX(0x55, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0x55, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0x55, 2)] = &mc_r_zpx_c2_data,
        [IDX(0x55, 3)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x55, 4)] = &mc_dispatch,
        [IDX(0x55, 5)] = &op_ILL,
        [IDX(0x55, 6)] = &op_ILL,
        [IDX(0x55, 7)] = &op_ILL,

        /* 0x56 LSR zp,X ; class=rw_zpx */
        [IDX(0x56, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0x56, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0x56, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0x56, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0x56, 4)] = &op_lsr_rw_ready_addr_data_pending_none_wr,
        [IDX(0x56, 5)] = &mc_fetch,
        [IDX(0x56, 6)] = &mc_dispatch,
        [IDX(0x56, 7)] = &op_ILL,

        /* 0x57 undocumented/illegal */
        [IDX(0x57, 0)] = &op_ILL,
        [IDX(0x57, 1)] = &op_ILL,
        [IDX(0x57, 2)] = &op_ILL,
        [IDX(0x57, 3)] = &op_ILL,
        [IDX(0x57, 4)] = &op_ILL,
        [IDX(0x57, 5)] = &op_ILL,
        [IDX(0x57, 6)] = &op_ILL,
        [IDX(0x57, 7)] = &op_ILL,

        /* 0x58 CLI ; class=imp */
        [IDX(0x58, 0)] = &op_cli_imp_ready_none_pending_none_dummy,
        [IDX(0x58, 1)] = &mc_fetch,
        [IDX(0x58, 2)] = &mc_dispatch,
        [IDX(0x58, 3)] = &op_ILL,
        [IDX(0x58, 4)] = &op_ILL,
        [IDX(0x58, 5)] = &op_ILL,
        [IDX(0x58, 6)] = &op_ILL,
        [IDX(0x58, 7)] = &op_ILL,

        /* 0x59 EOR abs,Y ; class=r_aby */
        [IDX(0x59, 0)] = &mc_r_aby_c0_lo,
        [IDX(0x59, 1)] = &mc_r_aby_c1_hi,
        [IDX(0x59, 2)] = &mc_r_aby_c2_probe,
        [IDX(0x59, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0x59, 4)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x59, 5)] = &mc_dispatch,
        [IDX(0x59, 6)] = &op_ILL,
        [IDX(0x59, 7)] = &op_ILL,

        /* 0x5A undocumented/illegal */
        [IDX(0x5A, 0)] = &op_ILL,
        [IDX(0x5A, 1)] = &op_ILL,
        [IDX(0x5A, 2)] = &op_ILL,
        [IDX(0x5A, 3)] = &op_ILL,
        [IDX(0x5A, 4)] = &op_ILL,
        [IDX(0x5A, 5)] = &op_ILL,
        [IDX(0x5A, 6)] = &op_ILL,
        [IDX(0x5A, 7)] = &op_ILL,

        /* 0x5B undocumented/illegal */
        [IDX(0x5B, 0)] = &op_ILL,
        [IDX(0x5B, 1)] = &op_ILL,
        [IDX(0x5B, 2)] = &op_ILL,
        [IDX(0x5B, 3)] = &op_ILL,
        [IDX(0x5B, 4)] = &op_ILL,
        [IDX(0x5B, 5)] = &op_ILL,
        [IDX(0x5B, 6)] = &op_ILL,
        [IDX(0x5B, 7)] = &op_ILL,

        /* 0x5C undocumented/illegal */
        [IDX(0x5C, 0)] = &op_ILL,
        [IDX(0x5C, 1)] = &op_ILL,
        [IDX(0x5C, 2)] = &op_ILL,
        [IDX(0x5C, 3)] = &op_ILL,
        [IDX(0x5C, 4)] = &op_ILL,
        [IDX(0x5C, 5)] = &op_ILL,
        [IDX(0x5C, 6)] = &op_ILL,
        [IDX(0x5C, 7)] = &op_ILL,

        /* 0x5D EOR abs,X ; class=r_abx */
        [IDX(0x5D, 0)] = &mc_r_abx_c0_lo,
        [IDX(0x5D, 1)] = &mc_r_abx_c1_hi,
        [IDX(0x5D, 2)] = &mc_r_abx_c2_probe,
        [IDX(0x5D, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0x5D, 4)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(0x5D, 5)] = &mc_dispatch,
        [IDX(0x5D, 6)] = &op_ILL,
        [IDX(0x5D, 7)] = &op_ILL,

        /* 0x5E LSR abs,X ; class=rw_abx */
        [IDX(0x5E, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0x5E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0x5E, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0x5E, 3)] = &mc_rw_abx_c3_data,
        [IDX(0x5E, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0x5E, 5)] = &op_lsr_rw_ready_addr_data_pending_none_wr,
        [IDX(0x5E, 6)] = &mc_fetch,
        [IDX(0x5E, 7)] = &mc_dispatch,

        /* 0x5F undocumented/illegal */
        [IDX(0x5F, 0)] = &op_ILL,
        [IDX(0x5F, 1)] = &op_ILL,
        [IDX(0x5F, 2)] = &op_ILL,
        [IDX(0x5F, 3)] = &op_ILL,
        [IDX(0x5F, 4)] = &op_ILL,
        [IDX(0x5F, 5)] = &op_ILL,
        [IDX(0x5F, 6)] = &op_ILL,
        [IDX(0x5F, 7)] = &op_ILL,

        /* 0x60 RTS ; class=rts */
        [IDX(0x60, 0)] = &mc_rts_c0_dummy,
        [IDX(0x60, 1)] = &mc_rts_c1_dummy,
        [IDX(0x60, 2)] = &mc_rts_c2_pull_pcl,
        [IDX(0x60, 3)] = &mc_rts_c3_pull_pch,
        [IDX(0x60, 4)] = &mc_rts_c4_inc_pc_dummy,
        [IDX(0x60, 5)] = &mc_fetch,
        [IDX(0x60, 6)] = &mc_dispatch,
        [IDX(0x60, 7)] = &op_ILL,

        /* 0x61 ADC (zp,X) ; class=r_izx */
        [IDX(0x61, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0x61, 1)] = &mc_r_izx_c1_idx,
        [IDX(0x61, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0x61, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0x61, 4)] = &mc_r_izx_c4_data,
        [IDX(0x61, 5)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x61, 6)] = &mc_dispatch,
        [IDX(0x61, 7)] = &op_ILL,

        /* 0x62 undocumented/illegal */
        [IDX(0x62, 0)] = &op_ILL,
        [IDX(0x62, 1)] = &op_ILL,
        [IDX(0x62, 2)] = &op_ILL,
        [IDX(0x62, 3)] = &op_ILL,
        [IDX(0x62, 4)] = &op_ILL,
        [IDX(0x62, 5)] = &op_ILL,
        [IDX(0x62, 6)] = &op_ILL,
        [IDX(0x62, 7)] = &op_ILL,

        /* 0x63 undocumented/illegal */
        [IDX(0x63, 0)] = &op_ILL,
        [IDX(0x63, 1)] = &op_ILL,
        [IDX(0x63, 2)] = &op_ILL,
        [IDX(0x63, 3)] = &op_ILL,
        [IDX(0x63, 4)] = &op_ILL,
        [IDX(0x63, 5)] = &op_ILL,
        [IDX(0x63, 6)] = &op_ILL,
        [IDX(0x63, 7)] = &op_ILL,

        /* 0x64 undocumented/illegal */
        [IDX(0x64, 0)] = &op_ILL,
        [IDX(0x64, 1)] = &op_ILL,
        [IDX(0x64, 2)] = &op_ILL,
        [IDX(0x64, 3)] = &op_ILL,
        [IDX(0x64, 4)] = &op_ILL,
        [IDX(0x64, 5)] = &op_ILL,
        [IDX(0x64, 6)] = &op_ILL,
        [IDX(0x64, 7)] = &op_ILL,

        /* 0x65 ADC zp ; class=r_zp */
        [IDX(0x65, 0)] = &mc_r_zp_c0_addr,
        [IDX(0x65, 1)] = &mc_r_zp_c1_data,
        [IDX(0x65, 2)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x65, 3)] = &mc_dispatch,
        [IDX(0x65, 4)] = &op_ILL,
        [IDX(0x65, 5)] = &op_ILL,
        [IDX(0x65, 6)] = &op_ILL,
        [IDX(0x65, 7)] = &op_ILL,

        /* 0x66 ROR zp ; class=rw_zp */
        [IDX(0x66, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0x66, 1)] = &mc_rw_zp_c1_data,
        [IDX(0x66, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0x66, 3)] = &op_ror_rw_ready_addr_data_pending_none_wr,
        [IDX(0x66, 4)] = &mc_fetch,
        [IDX(0x66, 5)] = &mc_dispatch,
        [IDX(0x66, 6)] = &op_ILL,
        [IDX(0x66, 7)] = &op_ILL,

        /* 0x67 undocumented/illegal */
        [IDX(0x67, 0)] = &op_ILL,
        [IDX(0x67, 1)] = &op_ILL,
        [IDX(0x67, 2)] = &op_ILL,
        [IDX(0x67, 3)] = &op_ILL,
        [IDX(0x67, 4)] = &op_ILL,
        [IDX(0x67, 5)] = &op_ILL,
        [IDX(0x67, 6)] = &op_ILL,
        [IDX(0x67, 7)] = &op_ILL,

        /* 0x68 PLA ; class=stack_pull */
        [IDX(0x68, 0)] = &mc_stack_pull_c0_dummy,
        [IDX(0x68, 1)] = &mc_stack_pull_c1_dummy,
        [IDX(0x68, 2)] = &mc_stack_pull_c2_read,
        [IDX(0x68, 3)] = &op_pla_stack_pull_ready_none_pending_data_fetch,
        [IDX(0x68, 4)] = &mc_dispatch,
        [IDX(0x68, 5)] = &op_ILL,
        [IDX(0x68, 6)] = &op_ILL,
        [IDX(0x68, 7)] = &op_ILL,

        /* 0x69 ADC #imm ; class=r_imm */
        [IDX(0x69, 0)] = &mc_r_imm_c0_data,
        [IDX(0x69, 1)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x69, 2)] = &mc_dispatch,
        [IDX(0x69, 3)] = &op_ILL,
        [IDX(0x69, 4)] = &op_ILL,
        [IDX(0x69, 5)] = &op_ILL,
        [IDX(0x69, 6)] = &op_ILL,
        [IDX(0x69, 7)] = &op_ILL,

        /* 0x6A ROR A ; class=acc */
        [IDX(0x6A, 0)] = &op_ror_acc_ready_none_pending_none_dummy,
        [IDX(0x6A, 1)] = &mc_fetch,
        [IDX(0x6A, 2)] = &mc_dispatch,
        [IDX(0x6A, 3)] = &op_ILL,
        [IDX(0x6A, 4)] = &op_ILL,
        [IDX(0x6A, 5)] = &op_ILL,
        [IDX(0x6A, 6)] = &op_ILL,
        [IDX(0x6A, 7)] = &op_ILL,

        /* 0x6B undocumented/illegal */
        [IDX(0x6B, 0)] = &op_ILL,
        [IDX(0x6B, 1)] = &op_ILL,
        [IDX(0x6B, 2)] = &op_ILL,
        [IDX(0x6B, 3)] = &op_ILL,
        [IDX(0x6B, 4)] = &op_ILL,
        [IDX(0x6B, 5)] = &op_ILL,
        [IDX(0x6B, 6)] = &op_ILL,
        [IDX(0x6B, 7)] = &op_ILL,

        /* 0x6C JMP (abs) ; class=jmp_ind */
        [IDX(0x6C, 0)] = &mc_jmp_ind_c0_ptrlo,
        [IDX(0x6C, 1)] = &mc_jmp_ind_c1_ptrhi,
        [IDX(0x6C, 2)] = &mc_jmp_ind_c2_target_lo,
        [IDX(0x6C, 3)] = &mc_jmp_ind_c3_target_hi,
        [IDX(0x6C, 4)] = &mc_jmp_ind_c4_fetch,
        [IDX(0x6C, 5)] = &mc_dispatch,
        [IDX(0x6C, 6)] = &op_ILL,
        [IDX(0x6C, 7)] = &op_ILL,

        /* 0x6D ADC abs ; class=r_abs */
        [IDX(0x6D, 0)] = &mc_r_abs_c0_lo,
        [IDX(0x6D, 1)] = &mc_r_abs_c1_hi,
        [IDX(0x6D, 2)] = &mc_r_abs_c2_data,
        [IDX(0x6D, 3)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x6D, 4)] = &mc_dispatch,
        [IDX(0x6D, 5)] = &op_ILL,
        [IDX(0x6D, 6)] = &op_ILL,
        [IDX(0x6D, 7)] = &op_ILL,

        /* 0x6E ROR abs ; class=rw_abs */
        [IDX(0x6E, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0x6E, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0x6E, 2)] = &mc_rw_abs_c2_data,
        [IDX(0x6E, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0x6E, 4)] = &op_ror_rw_ready_addr_data_pending_none_wr,
        [IDX(0x6E, 5)] = &mc_fetch,
        [IDX(0x6E, 6)] = &mc_dispatch,
        [IDX(0x6E, 7)] = &op_ILL,

        /* 0x6F undocumented/illegal */
        [IDX(0x6F, 0)] = &op_ILL,
        [IDX(0x6F, 1)] = &op_ILL,
        [IDX(0x6F, 2)] = &op_ILL,
        [IDX(0x6F, 3)] = &op_ILL,
        [IDX(0x6F, 4)] = &op_ILL,
        [IDX(0x6F, 5)] = &op_ILL,
        [IDX(0x6F, 6)] = &op_ILL,
        [IDX(0x6F, 7)] = &op_ILL,

        /* 0x70 BVS rel ; class=branch_rel */
        [IDX(0x70, 0)] = &op_bvs_branch_c0_offset,
        [IDX(0x70, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0x70, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0x70, 3)] = &mc_fetch,
        [IDX(0x70, 4)] = &mc_dispatch,
        [IDX(0x70, 5)] = &op_ILL,
        [IDX(0x70, 6)] = &op_ILL,
        [IDX(0x70, 7)] = &op_ILL,

        /* 0x71 ADC (zp),Y ; class=r_izy */
        [IDX(0x71, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0x71, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0x71, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0x71, 3)] = &mc_r_izy_c3_probe,
        [IDX(0x71, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0x71, 5)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x71, 6)] = &mc_dispatch,
        [IDX(0x71, 7)] = &op_ILL,

        /* 0x72 undocumented/illegal */
        [IDX(0x72, 0)] = &op_ILL,
        [IDX(0x72, 1)] = &op_ILL,
        [IDX(0x72, 2)] = &op_ILL,
        [IDX(0x72, 3)] = &op_ILL,
        [IDX(0x72, 4)] = &op_ILL,
        [IDX(0x72, 5)] = &op_ILL,
        [IDX(0x72, 6)] = &op_ILL,
        [IDX(0x72, 7)] = &op_ILL,

        /* 0x73 undocumented/illegal */
        [IDX(0x73, 0)] = &op_ILL,
        [IDX(0x73, 1)] = &op_ILL,
        [IDX(0x73, 2)] = &op_ILL,
        [IDX(0x73, 3)] = &op_ILL,
        [IDX(0x73, 4)] = &op_ILL,
        [IDX(0x73, 5)] = &op_ILL,
        [IDX(0x73, 6)] = &op_ILL,
        [IDX(0x73, 7)] = &op_ILL,

        /* 0x74 undocumented/illegal */
        [IDX(0x74, 0)] = &op_ILL,
        [IDX(0x74, 1)] = &op_ILL,
        [IDX(0x74, 2)] = &op_ILL,
        [IDX(0x74, 3)] = &op_ILL,
        [IDX(0x74, 4)] = &op_ILL,
        [IDX(0x74, 5)] = &op_ILL,
        [IDX(0x74, 6)] = &op_ILL,
        [IDX(0x74, 7)] = &op_ILL,

        /* 0x75 ADC zp,X ; class=r_zpx */
        [IDX(0x75, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0x75, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0x75, 2)] = &mc_r_zpx_c2_data,
        [IDX(0x75, 3)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x75, 4)] = &mc_dispatch,
        [IDX(0x75, 5)] = &op_ILL,
        [IDX(0x75, 6)] = &op_ILL,
        [IDX(0x75, 7)] = &op_ILL,

        /* 0x76 ROR zp,X ; class=rw_zpx */
        [IDX(0x76, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0x76, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0x76, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0x76, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0x76, 4)] = &op_ror_rw_ready_addr_data_pending_none_wr,
        [IDX(0x76, 5)] = &mc_fetch,
        [IDX(0x76, 6)] = &mc_dispatch,
        [IDX(0x76, 7)] = &op_ILL,

        /* 0x77 undocumented/illegal */
        [IDX(0x77, 0)] = &op_ILL,
        [IDX(0x77, 1)] = &op_ILL,
        [IDX(0x77, 2)] = &op_ILL,
        [IDX(0x77, 3)] = &op_ILL,
        [IDX(0x77, 4)] = &op_ILL,
        [IDX(0x77, 5)] = &op_ILL,
        [IDX(0x77, 6)] = &op_ILL,
        [IDX(0x77, 7)] = &op_ILL,

        /* 0x78 SEI ; class=imp */
        [IDX(0x78, 0)] = &op_sei_imp_ready_none_pending_none_dummy,
        [IDX(0x78, 1)] = &mc_fetch,
        [IDX(0x78, 2)] = &mc_dispatch,
        [IDX(0x78, 3)] = &op_ILL,
        [IDX(0x78, 4)] = &op_ILL,
        [IDX(0x78, 5)] = &op_ILL,
        [IDX(0x78, 6)] = &op_ILL,
        [IDX(0x78, 7)] = &op_ILL,

        /* 0x79 ADC abs,Y ; class=r_aby */
        [IDX(0x79, 0)] = &mc_r_aby_c0_lo,
        [IDX(0x79, 1)] = &mc_r_aby_c1_hi,
        [IDX(0x79, 2)] = &mc_r_aby_c2_probe,
        [IDX(0x79, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0x79, 4)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x79, 5)] = &mc_dispatch,
        [IDX(0x79, 6)] = &op_ILL,
        [IDX(0x79, 7)] = &op_ILL,

        /* 0x7A undocumented/illegal */
        [IDX(0x7A, 0)] = &op_ILL,
        [IDX(0x7A, 1)] = &op_ILL,
        [IDX(0x7A, 2)] = &op_ILL,
        [IDX(0x7A, 3)] = &op_ILL,
        [IDX(0x7A, 4)] = &op_ILL,
        [IDX(0x7A, 5)] = &op_ILL,
        [IDX(0x7A, 6)] = &op_ILL,
        [IDX(0x7A, 7)] = &op_ILL,

        /* 0x7B undocumented/illegal */
        [IDX(0x7B, 0)] = &op_ILL,
        [IDX(0x7B, 1)] = &op_ILL,
        [IDX(0x7B, 2)] = &op_ILL,
        [IDX(0x7B, 3)] = &op_ILL,
        [IDX(0x7B, 4)] = &op_ILL,
        [IDX(0x7B, 5)] = &op_ILL,
        [IDX(0x7B, 6)] = &op_ILL,
        [IDX(0x7B, 7)] = &op_ILL,

        /* 0x7C undocumented/illegal */
        [IDX(0x7C, 0)] = &op_ILL,
        [IDX(0x7C, 1)] = &op_ILL,
        [IDX(0x7C, 2)] = &op_ILL,
        [IDX(0x7C, 3)] = &op_ILL,
        [IDX(0x7C, 4)] = &op_ILL,
        [IDX(0x7C, 5)] = &op_ILL,
        [IDX(0x7C, 6)] = &op_ILL,
        [IDX(0x7C, 7)] = &op_ILL,

        /* 0x7D ADC abs,X ; class=r_abx */
        [IDX(0x7D, 0)] = &mc_r_abx_c0_lo,
        [IDX(0x7D, 1)] = &mc_r_abx_c1_hi,
        [IDX(0x7D, 2)] = &mc_r_abx_c2_probe,
        [IDX(0x7D, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0x7D, 4)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(0x7D, 5)] = &mc_dispatch,
        [IDX(0x7D, 6)] = &op_ILL,
        [IDX(0x7D, 7)] = &op_ILL,

        /* 0x7E ROR abs,X ; class=rw_abx */
        [IDX(0x7E, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0x7E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0x7E, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0x7E, 3)] = &mc_rw_abx_c3_data,
        [IDX(0x7E, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0x7E, 5)] = &op_ror_rw_ready_addr_data_pending_none_wr,
        [IDX(0x7E, 6)] = &mc_fetch,
        [IDX(0x7E, 7)] = &mc_dispatch,

        /* 0x7F undocumented/illegal */
        [IDX(0x7F, 0)] = &op_ILL,
        [IDX(0x7F, 1)] = &op_ILL,
        [IDX(0x7F, 2)] = &op_ILL,
        [IDX(0x7F, 3)] = &op_ILL,
        [IDX(0x7F, 4)] = &op_ILL,
        [IDX(0x7F, 5)] = &op_ILL,
        [IDX(0x7F, 6)] = &op_ILL,
        [IDX(0x7F, 7)] = &op_ILL,

        /* 0x80 undocumented/illegal */
        [IDX(0x80, 0)] = &op_ILL,
        [IDX(0x80, 1)] = &op_ILL,
        [IDX(0x80, 2)] = &op_ILL,
        [IDX(0x80, 3)] = &op_ILL,
        [IDX(0x80, 4)] = &op_ILL,
        [IDX(0x80, 5)] = &op_ILL,
        [IDX(0x80, 6)] = &op_ILL,
        [IDX(0x80, 7)] = &op_ILL,

        /* 0x81 STA (zp,X) ; class=w_izx */
        [IDX(0x81, 0)] = &mc_w_izx_c0_ptr,
        [IDX(0x81, 1)] = &mc_w_izx_c1_idx,
        [IDX(0x81, 2)] = &mc_w_izx_c2_ptrlo,
        [IDX(0x81, 3)] = &mc_w_izx_c3_ptrhi,
        [IDX(0x81, 4)] = &op_sta_w_ready_addrlo_pending_addrhi_wr,
        [IDX(0x81, 5)] = &mc_fetch,
        [IDX(0x81, 6)] = &mc_dispatch,
        [IDX(0x81, 7)] = &op_ILL,

        /* 0x82 undocumented/illegal */
        [IDX(0x82, 0)] = &op_ILL,
        [IDX(0x82, 1)] = &op_ILL,
        [IDX(0x82, 2)] = &op_ILL,
        [IDX(0x82, 3)] = &op_ILL,
        [IDX(0x82, 4)] = &op_ILL,
        [IDX(0x82, 5)] = &op_ILL,
        [IDX(0x82, 6)] = &op_ILL,
        [IDX(0x82, 7)] = &op_ILL,

        /* 0x83 undocumented/illegal */
        [IDX(0x83, 0)] = &op_ILL,
        [IDX(0x83, 1)] = &op_ILL,
        [IDX(0x83, 2)] = &op_ILL,
        [IDX(0x83, 3)] = &op_ILL,
        [IDX(0x83, 4)] = &op_ILL,
        [IDX(0x83, 5)] = &op_ILL,
        [IDX(0x83, 6)] = &op_ILL,
        [IDX(0x83, 7)] = &op_ILL,

        /* 0x84 STY zp ; class=w_zp */
        [IDX(0x84, 0)] = &mc_w_zp_c0_addr,
        [IDX(0x84, 1)] = &op_sty_w_ready_none_pending_addrlo_wr,
        [IDX(0x84, 2)] = &mc_fetch,
        [IDX(0x84, 3)] = &mc_dispatch,
        [IDX(0x84, 4)] = &op_ILL,
        [IDX(0x84, 5)] = &op_ILL,
        [IDX(0x84, 6)] = &op_ILL,
        [IDX(0x84, 7)] = &op_ILL,

        /* 0x85 STA zp ; class=w_zp */
        [IDX(0x85, 0)] = &mc_w_zp_c0_addr,
        [IDX(0x85, 1)] = &op_sta_w_ready_none_pending_addrlo_wr,
        [IDX(0x85, 2)] = &mc_fetch,
        [IDX(0x85, 3)] = &mc_dispatch,
        [IDX(0x85, 4)] = &op_ILL,
        [IDX(0x85, 5)] = &op_ILL,
        [IDX(0x85, 6)] = &op_ILL,
        [IDX(0x85, 7)] = &op_ILL,

        /* 0x86 STX zp ; class=w_zp */
        [IDX(0x86, 0)] = &mc_w_zp_c0_addr,
        [IDX(0x86, 1)] = &op_stx_w_ready_none_pending_addrlo_wr,
        [IDX(0x86, 2)] = &mc_fetch,
        [IDX(0x86, 3)] = &mc_dispatch,
        [IDX(0x86, 4)] = &op_ILL,
        [IDX(0x86, 5)] = &op_ILL,
        [IDX(0x86, 6)] = &op_ILL,
        [IDX(0x86, 7)] = &op_ILL,

        /* 0x87 undocumented/illegal */
        [IDX(0x87, 0)] = &op_ILL,
        [IDX(0x87, 1)] = &op_ILL,
        [IDX(0x87, 2)] = &op_ILL,
        [IDX(0x87, 3)] = &op_ILL,
        [IDX(0x87, 4)] = &op_ILL,
        [IDX(0x87, 5)] = &op_ILL,
        [IDX(0x87, 6)] = &op_ILL,
        [IDX(0x87, 7)] = &op_ILL,

        /* 0x88 DEY ; class=imp */
        [IDX(0x88, 0)] = &op_dey_imp_ready_none_pending_none_dummy,
        [IDX(0x88, 1)] = &mc_fetch,
        [IDX(0x88, 2)] = &mc_dispatch,
        [IDX(0x88, 3)] = &op_ILL,
        [IDX(0x88, 4)] = &op_ILL,
        [IDX(0x88, 5)] = &op_ILL,
        [IDX(0x88, 6)] = &op_ILL,
        [IDX(0x88, 7)] = &op_ILL,

        /* 0x89 undocumented/illegal */
        [IDX(0x89, 0)] = &op_ILL,
        [IDX(0x89, 1)] = &op_ILL,
        [IDX(0x89, 2)] = &op_ILL,
        [IDX(0x89, 3)] = &op_ILL,
        [IDX(0x89, 4)] = &op_ILL,
        [IDX(0x89, 5)] = &op_ILL,
        [IDX(0x89, 6)] = &op_ILL,
        [IDX(0x89, 7)] = &op_ILL,

        /* 0x8A TXA ; class=imp */
        [IDX(0x8A, 0)] = &op_txa_imp_ready_none_pending_none_dummy,
        [IDX(0x8A, 1)] = &mc_fetch,
        [IDX(0x8A, 2)] = &mc_dispatch,
        [IDX(0x8A, 3)] = &op_ILL,
        [IDX(0x8A, 4)] = &op_ILL,
        [IDX(0x8A, 5)] = &op_ILL,
        [IDX(0x8A, 6)] = &op_ILL,
        [IDX(0x8A, 7)] = &op_ILL,

        /* 0x8B undocumented/illegal */
        [IDX(0x8B, 0)] = &op_ILL,
        [IDX(0x8B, 1)] = &op_ILL,
        [IDX(0x8B, 2)] = &op_ILL,
        [IDX(0x8B, 3)] = &op_ILL,
        [IDX(0x8B, 4)] = &op_ILL,
        [IDX(0x8B, 5)] = &op_ILL,
        [IDX(0x8B, 6)] = &op_ILL,
        [IDX(0x8B, 7)] = &op_ILL,

        /* 0x8C STY abs ; class=w_abs */
        [IDX(0x8C, 0)] = &mc_w_abs_c0_lo,
        [IDX(0x8C, 1)] = &mc_w_abs_c1_hi,
        [IDX(0x8C, 2)] = &op_sty_w_ready_addrlo_pending_addrhi_wr,
        [IDX(0x8C, 3)] = &mc_fetch,
        [IDX(0x8C, 4)] = &mc_dispatch,
        [IDX(0x8C, 5)] = &op_ILL,
        [IDX(0x8C, 6)] = &op_ILL,
        [IDX(0x8C, 7)] = &op_ILL,

        /* 0x8D STA abs ; class=w_abs */
        [IDX(0x8D, 0)] = &mc_w_abs_c0_lo,
        [IDX(0x8D, 1)] = &mc_w_abs_c1_hi,
        [IDX(0x8D, 2)] = &op_sta_w_ready_addrlo_pending_addrhi_wr,
        [IDX(0x8D, 3)] = &mc_fetch,
        [IDX(0x8D, 4)] = &mc_dispatch,
        [IDX(0x8D, 5)] = &op_ILL,
        [IDX(0x8D, 6)] = &op_ILL,
        [IDX(0x8D, 7)] = &op_ILL,

        /* 0x8E STX abs ; class=w_abs */
        [IDX(0x8E, 0)] = &mc_w_abs_c0_lo,
        [IDX(0x8E, 1)] = &mc_w_abs_c1_hi,
        [IDX(0x8E, 2)] = &op_stx_w_ready_addrlo_pending_addrhi_wr,
        [IDX(0x8E, 3)] = &mc_fetch,
        [IDX(0x8E, 4)] = &mc_dispatch,
        [IDX(0x8E, 5)] = &op_ILL,
        [IDX(0x8E, 6)] = &op_ILL,
        [IDX(0x8E, 7)] = &op_ILL,

        /* 0x8F undocumented/illegal */
        [IDX(0x8F, 0)] = &op_ILL,
        [IDX(0x8F, 1)] = &op_ILL,
        [IDX(0x8F, 2)] = &op_ILL,
        [IDX(0x8F, 3)] = &op_ILL,
        [IDX(0x8F, 4)] = &op_ILL,
        [IDX(0x8F, 5)] = &op_ILL,
        [IDX(0x8F, 6)] = &op_ILL,
        [IDX(0x8F, 7)] = &op_ILL,

        /* 0x90 BCC rel ; class=branch_rel */
        [IDX(0x90, 0)] = &op_bcc_branch_c0_offset,
        [IDX(0x90, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0x90, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0x90, 3)] = &mc_fetch,
        [IDX(0x90, 4)] = &mc_dispatch,
        [IDX(0x90, 5)] = &op_ILL,
        [IDX(0x90, 6)] = &op_ILL,
        [IDX(0x90, 7)] = &op_ILL,

        /* 0x91 STA (zp),Y ; class=w_izy */
        [IDX(0x91, 0)] = &mc_w_izy_c0_ptr,
        [IDX(0x91, 1)] = &mc_w_izy_c1_ptrlo,
        [IDX(0x91, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(0x91, 3)] = &mc_w_izy_c3_probe,
        [IDX(0x91, 4)] = &op_sta_w_ready_addr_pending_none_wr,
        [IDX(0x91, 5)] = &mc_fetch,
        [IDX(0x91, 6)] = &mc_dispatch,
        [IDX(0x91, 7)] = &op_ILL,

        /* 0x92 undocumented/illegal */
        [IDX(0x92, 0)] = &op_ILL,
        [IDX(0x92, 1)] = &op_ILL,
        [IDX(0x92, 2)] = &op_ILL,
        [IDX(0x92, 3)] = &op_ILL,
        [IDX(0x92, 4)] = &op_ILL,
        [IDX(0x92, 5)] = &op_ILL,
        [IDX(0x92, 6)] = &op_ILL,
        [IDX(0x92, 7)] = &op_ILL,

        /* 0x93 undocumented/illegal */
        [IDX(0x93, 0)] = &op_ILL,
        [IDX(0x93, 1)] = &op_ILL,
        [IDX(0x93, 2)] = &op_ILL,
        [IDX(0x93, 3)] = &op_ILL,
        [IDX(0x93, 4)] = &op_ILL,
        [IDX(0x93, 5)] = &op_ILL,
        [IDX(0x93, 6)] = &op_ILL,
        [IDX(0x93, 7)] = &op_ILL,

        /* 0x94 STY zp,X ; class=w_zpx */
        [IDX(0x94, 0)] = &mc_w_zpx_c0_addr,
        [IDX(0x94, 1)] = &mc_w_zpx_c1_idx,
        [IDX(0x94, 2)] = &op_sty_w_ready_addr_pending_none_wr,
        [IDX(0x94, 3)] = &mc_fetch,
        [IDX(0x94, 4)] = &mc_dispatch,
        [IDX(0x94, 5)] = &op_ILL,
        [IDX(0x94, 6)] = &op_ILL,
        [IDX(0x94, 7)] = &op_ILL,

        /* 0x95 STA zp,X ; class=w_zpx */
        [IDX(0x95, 0)] = &mc_w_zpx_c0_addr,
        [IDX(0x95, 1)] = &mc_w_zpx_c1_idx,
        [IDX(0x95, 2)] = &op_sta_w_ready_addr_pending_none_wr,
        [IDX(0x95, 3)] = &mc_fetch,
        [IDX(0x95, 4)] = &mc_dispatch,
        [IDX(0x95, 5)] = &op_ILL,
        [IDX(0x95, 6)] = &op_ILL,
        [IDX(0x95, 7)] = &op_ILL,

        /* 0x96 STX zp,Y ; class=w_zpy */
        [IDX(0x96, 0)] = &mc_w_zpy_c0_addr,
        [IDX(0x96, 1)] = &mc_w_zpy_c1_idx,
        [IDX(0x96, 2)] = &op_stx_w_ready_addr_pending_none_wr,
        [IDX(0x96, 3)] = &mc_fetch,
        [IDX(0x96, 4)] = &mc_dispatch,
        [IDX(0x96, 5)] = &op_ILL,
        [IDX(0x96, 6)] = &op_ILL,
        [IDX(0x96, 7)] = &op_ILL,

        /* 0x97 undocumented/illegal */
        [IDX(0x97, 0)] = &op_ILL,
        [IDX(0x97, 1)] = &op_ILL,
        [IDX(0x97, 2)] = &op_ILL,
        [IDX(0x97, 3)] = &op_ILL,
        [IDX(0x97, 4)] = &op_ILL,
        [IDX(0x97, 5)] = &op_ILL,
        [IDX(0x97, 6)] = &op_ILL,
        [IDX(0x97, 7)] = &op_ILL,

        /* 0x98 TYA ; class=imp */
        [IDX(0x98, 0)] = &op_tya_imp_ready_none_pending_none_dummy,
        [IDX(0x98, 1)] = &mc_fetch,
        [IDX(0x98, 2)] = &mc_dispatch,
        [IDX(0x98, 3)] = &op_ILL,
        [IDX(0x98, 4)] = &op_ILL,
        [IDX(0x98, 5)] = &op_ILL,
        [IDX(0x98, 6)] = &op_ILL,
        [IDX(0x98, 7)] = &op_ILL,

        /* 0x99 STA abs,Y ; class=w_aby */
        [IDX(0x99, 0)] = &mc_w_aby_c0_lo,
        [IDX(0x99, 1)] = &mc_w_aby_c1_hi,
        [IDX(0x99, 2)] = &mc_w_aby_c2_probe,
        [IDX(0x99, 3)] = &op_sta_w_ready_addr_pending_none_wr,
        [IDX(0x99, 4)] = &mc_fetch,
        [IDX(0x99, 5)] = &mc_dispatch,
        [IDX(0x99, 6)] = &op_ILL,
        [IDX(0x99, 7)] = &op_ILL,

        /* 0x9A TXS ; class=imp */
        [IDX(0x9A, 0)] = &op_txs_imp_ready_none_pending_none_dummy,
        [IDX(0x9A, 1)] = &mc_fetch,
        [IDX(0x9A, 2)] = &mc_dispatch,
        [IDX(0x9A, 3)] = &op_ILL,
        [IDX(0x9A, 4)] = &op_ILL,
        [IDX(0x9A, 5)] = &op_ILL,
        [IDX(0x9A, 6)] = &op_ILL,
        [IDX(0x9A, 7)] = &op_ILL,

        /* 0x9B undocumented/illegal */
        [IDX(0x9B, 0)] = &op_ILL,
        [IDX(0x9B, 1)] = &op_ILL,
        [IDX(0x9B, 2)] = &op_ILL,
        [IDX(0x9B, 3)] = &op_ILL,
        [IDX(0x9B, 4)] = &op_ILL,
        [IDX(0x9B, 5)] = &op_ILL,
        [IDX(0x9B, 6)] = &op_ILL,
        [IDX(0x9B, 7)] = &op_ILL,

        /* 0x9C undocumented/illegal */
        [IDX(0x9C, 0)] = &op_ILL,
        [IDX(0x9C, 1)] = &op_ILL,
        [IDX(0x9C, 2)] = &op_ILL,
        [IDX(0x9C, 3)] = &op_ILL,
        [IDX(0x9C, 4)] = &op_ILL,
        [IDX(0x9C, 5)] = &op_ILL,
        [IDX(0x9C, 6)] = &op_ILL,
        [IDX(0x9C, 7)] = &op_ILL,

        /* 0x9D STA abs,X ; class=w_abx */
        [IDX(0x9D, 0)] = &mc_w_abx_c0_lo,
        [IDX(0x9D, 1)] = &mc_w_abx_c1_hi,
        [IDX(0x9D, 2)] = &mc_w_abx_c2_probe,
        [IDX(0x9D, 3)] = &op_sta_w_ready_addr_pending_none_wr,
        [IDX(0x9D, 4)] = &mc_fetch,
        [IDX(0x9D, 5)] = &mc_dispatch,
        [IDX(0x9D, 6)] = &op_ILL,
        [IDX(0x9D, 7)] = &op_ILL,

        /* 0x9E undocumented/illegal */
        [IDX(0x9E, 0)] = &op_ILL,
        [IDX(0x9E, 1)] = &op_ILL,
        [IDX(0x9E, 2)] = &op_ILL,
        [IDX(0x9E, 3)] = &op_ILL,
        [IDX(0x9E, 4)] = &op_ILL,
        [IDX(0x9E, 5)] = &op_ILL,
        [IDX(0x9E, 6)] = &op_ILL,
        [IDX(0x9E, 7)] = &op_ILL,

        /* 0x9F undocumented/illegal */
        [IDX(0x9F, 0)] = &op_ILL,
        [IDX(0x9F, 1)] = &op_ILL,
        [IDX(0x9F, 2)] = &op_ILL,
        [IDX(0x9F, 3)] = &op_ILL,
        [IDX(0x9F, 4)] = &op_ILL,
        [IDX(0x9F, 5)] = &op_ILL,
        [IDX(0x9F, 6)] = &op_ILL,
        [IDX(0x9F, 7)] = &op_ILL,

        /* 0xA0 LDY #imm ; class=r_imm */
        [IDX(0xA0, 0)] = &mc_r_imm_c0_data,
        [IDX(0xA0, 1)] = &op_ldy_r_ready_none_pending_data_fetch,
        [IDX(0xA0, 2)] = &mc_dispatch,
        [IDX(0xA0, 3)] = &op_ILL,
        [IDX(0xA0, 4)] = &op_ILL,
        [IDX(0xA0, 5)] = &op_ILL,
        [IDX(0xA0, 6)] = &op_ILL,
        [IDX(0xA0, 7)] = &op_ILL,

        /* 0xA1 LDA (zp,X) ; class=r_izx */
        [IDX(0xA1, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0xA1, 1)] = &mc_r_izx_c1_idx,
        [IDX(0xA1, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0xA1, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0xA1, 4)] = &mc_r_izx_c4_data,
        [IDX(0xA1, 5)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xA1, 6)] = &mc_dispatch,
        [IDX(0xA1, 7)] = &op_ILL,

        /* 0xA2 LDX #imm ; class=r_imm */
        [IDX(0xA2, 0)] = &mc_r_imm_c0_data,
        [IDX(0xA2, 1)] = &op_ldx_r_ready_none_pending_data_fetch,
        [IDX(0xA2, 2)] = &mc_dispatch,
        [IDX(0xA2, 3)] = &op_ILL,
        [IDX(0xA2, 4)] = &op_ILL,
        [IDX(0xA2, 5)] = &op_ILL,
        [IDX(0xA2, 6)] = &op_ILL,
        [IDX(0xA2, 7)] = &op_ILL,

        /* 0xA3 undocumented/illegal */
        [IDX(0xA3, 0)] = &op_ILL,
        [IDX(0xA3, 1)] = &op_ILL,
        [IDX(0xA3, 2)] = &op_ILL,
        [IDX(0xA3, 3)] = &op_ILL,
        [IDX(0xA3, 4)] = &op_ILL,
        [IDX(0xA3, 5)] = &op_ILL,
        [IDX(0xA3, 6)] = &op_ILL,
        [IDX(0xA3, 7)] = &op_ILL,

        /* 0xA4 LDY zp ; class=r_zp */
        [IDX(0xA4, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xA4, 1)] = &mc_r_zp_c1_data,
        [IDX(0xA4, 2)] = &op_ldy_r_ready_none_pending_data_fetch,
        [IDX(0xA4, 3)] = &mc_dispatch,
        [IDX(0xA4, 4)] = &op_ILL,
        [IDX(0xA4, 5)] = &op_ILL,
        [IDX(0xA4, 6)] = &op_ILL,
        [IDX(0xA4, 7)] = &op_ILL,

        /* 0xA5 LDA zp ; class=r_zp */
        [IDX(0xA5, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xA5, 1)] = &mc_r_zp_c1_data,
        [IDX(0xA5, 2)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xA5, 3)] = &mc_dispatch,
        [IDX(0xA5, 4)] = &op_ILL,
        [IDX(0xA5, 5)] = &op_ILL,
        [IDX(0xA5, 6)] = &op_ILL,
        [IDX(0xA5, 7)] = &op_ILL,

        /* 0xA6 LDX zp ; class=r_zp */
        [IDX(0xA6, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xA6, 1)] = &mc_r_zp_c1_data,
        [IDX(0xA6, 2)] = &op_ldx_r_ready_none_pending_data_fetch,
        [IDX(0xA6, 3)] = &mc_dispatch,
        [IDX(0xA6, 4)] = &op_ILL,
        [IDX(0xA6, 5)] = &op_ILL,
        [IDX(0xA6, 6)] = &op_ILL,
        [IDX(0xA6, 7)] = &op_ILL,

        /* 0xA7 undocumented/illegal */
        [IDX(0xA7, 0)] = &op_ILL,
        [IDX(0xA7, 1)] = &op_ILL,
        [IDX(0xA7, 2)] = &op_ILL,
        [IDX(0xA7, 3)] = &op_ILL,
        [IDX(0xA7, 4)] = &op_ILL,
        [IDX(0xA7, 5)] = &op_ILL,
        [IDX(0xA7, 6)] = &op_ILL,
        [IDX(0xA7, 7)] = &op_ILL,

        /* 0xA8 TAY ; class=imp */
        [IDX(0xA8, 0)] = &op_tay_imp_ready_none_pending_none_dummy,
        [IDX(0xA8, 1)] = &mc_fetch,
        [IDX(0xA8, 2)] = &mc_dispatch,
        [IDX(0xA8, 3)] = &op_ILL,
        [IDX(0xA8, 4)] = &op_ILL,
        [IDX(0xA8, 5)] = &op_ILL,
        [IDX(0xA8, 6)] = &op_ILL,
        [IDX(0xA8, 7)] = &op_ILL,

        /* 0xA9 LDA #imm ; class=r_imm */
        [IDX(0xA9, 0)] = &mc_r_imm_c0_data,
        [IDX(0xA9, 1)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xA9, 2)] = &mc_dispatch,
        [IDX(0xA9, 3)] = &op_ILL,
        [IDX(0xA9, 4)] = &op_ILL,
        [IDX(0xA9, 5)] = &op_ILL,
        [IDX(0xA9, 6)] = &op_ILL,
        [IDX(0xA9, 7)] = &op_ILL,

        /* 0xAA TAX ; class=imp */
        [IDX(0xAA, 0)] = &op_tax_imp_ready_none_pending_none_dummy,
        [IDX(0xAA, 1)] = &mc_fetch,
        [IDX(0xAA, 2)] = &mc_dispatch,
        [IDX(0xAA, 3)] = &op_ILL,
        [IDX(0xAA, 4)] = &op_ILL,
        [IDX(0xAA, 5)] = &op_ILL,
        [IDX(0xAA, 6)] = &op_ILL,
        [IDX(0xAA, 7)] = &op_ILL,

        /* 0xAB undocumented/illegal */
        [IDX(0xAB, 0)] = &op_ILL,
        [IDX(0xAB, 1)] = &op_ILL,
        [IDX(0xAB, 2)] = &op_ILL,
        [IDX(0xAB, 3)] = &op_ILL,
        [IDX(0xAB, 4)] = &op_ILL,
        [IDX(0xAB, 5)] = &op_ILL,
        [IDX(0xAB, 6)] = &op_ILL,
        [IDX(0xAB, 7)] = &op_ILL,

        /* 0xAC LDY abs ; class=r_abs */
        [IDX(0xAC, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xAC, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xAC, 2)] = &mc_r_abs_c2_data,
        [IDX(0xAC, 3)] = &op_ldy_r_ready_none_pending_data_fetch,
        [IDX(0xAC, 4)] = &mc_dispatch,
        [IDX(0xAC, 5)] = &op_ILL,
        [IDX(0xAC, 6)] = &op_ILL,
        [IDX(0xAC, 7)] = &op_ILL,

        /* 0xAD LDA abs ; class=r_abs */
        [IDX(0xAD, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xAD, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xAD, 2)] = &mc_r_abs_c2_data,
        [IDX(0xAD, 3)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xAD, 4)] = &mc_dispatch,
        [IDX(0xAD, 5)] = &op_ILL,
        [IDX(0xAD, 6)] = &op_ILL,
        [IDX(0xAD, 7)] = &op_ILL,

        /* 0xAE LDX abs ; class=r_abs */
        [IDX(0xAE, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xAE, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xAE, 2)] = &mc_r_abs_c2_data,
        [IDX(0xAE, 3)] = &op_ldx_r_ready_none_pending_data_fetch,
        [IDX(0xAE, 4)] = &mc_dispatch,
        [IDX(0xAE, 5)] = &op_ILL,
        [IDX(0xAE, 6)] = &op_ILL,
        [IDX(0xAE, 7)] = &op_ILL,

        /* 0xAF undocumented/illegal */
        [IDX(0xAF, 0)] = &op_ILL,
        [IDX(0xAF, 1)] = &op_ILL,
        [IDX(0xAF, 2)] = &op_ILL,
        [IDX(0xAF, 3)] = &op_ILL,
        [IDX(0xAF, 4)] = &op_ILL,
        [IDX(0xAF, 5)] = &op_ILL,
        [IDX(0xAF, 6)] = &op_ILL,
        [IDX(0xAF, 7)] = &op_ILL,

        /* 0xB0 BCS rel ; class=branch_rel */
        [IDX(0xB0, 0)] = &op_bcs_branch_c0_offset,
        [IDX(0xB0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0xB0, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0xB0, 3)] = &mc_fetch,
        [IDX(0xB0, 4)] = &mc_dispatch,
        [IDX(0xB0, 5)] = &op_ILL,
        [IDX(0xB0, 6)] = &op_ILL,
        [IDX(0xB0, 7)] = &op_ILL,

        /* 0xB1 LDA (zp),Y ; class=r_izy */
        [IDX(0xB1, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0xB1, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0xB1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0xB1, 3)] = &mc_r_izy_c3_probe,
        [IDX(0xB1, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0xB1, 5)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xB1, 6)] = &mc_dispatch,
        [IDX(0xB1, 7)] = &op_ILL,

        /* 0xB2 undocumented/illegal */
        [IDX(0xB2, 0)] = &op_ILL,
        [IDX(0xB2, 1)] = &op_ILL,
        [IDX(0xB2, 2)] = &op_ILL,
        [IDX(0xB2, 3)] = &op_ILL,
        [IDX(0xB2, 4)] = &op_ILL,
        [IDX(0xB2, 5)] = &op_ILL,
        [IDX(0xB2, 6)] = &op_ILL,
        [IDX(0xB2, 7)] = &op_ILL,

        /* 0xB3 undocumented/illegal */
        [IDX(0xB3, 0)] = &op_ILL,
        [IDX(0xB3, 1)] = &op_ILL,
        [IDX(0xB3, 2)] = &op_ILL,
        [IDX(0xB3, 3)] = &op_ILL,
        [IDX(0xB3, 4)] = &op_ILL,
        [IDX(0xB3, 5)] = &op_ILL,
        [IDX(0xB3, 6)] = &op_ILL,
        [IDX(0xB3, 7)] = &op_ILL,

        /* 0xB4 LDY zp,X ; class=r_zpx */
        [IDX(0xB4, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0xB4, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0xB4, 2)] = &mc_r_zpx_c2_data,
        [IDX(0xB4, 3)] = &op_ldy_r_ready_none_pending_data_fetch,
        [IDX(0xB4, 4)] = &mc_dispatch,
        [IDX(0xB4, 5)] = &op_ILL,
        [IDX(0xB4, 6)] = &op_ILL,
        [IDX(0xB4, 7)] = &op_ILL,

        /* 0xB5 LDA zp,X ; class=r_zpx */
        [IDX(0xB5, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0xB5, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0xB5, 2)] = &mc_r_zpx_c2_data,
        [IDX(0xB5, 3)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xB5, 4)] = &mc_dispatch,
        [IDX(0xB5, 5)] = &op_ILL,
        [IDX(0xB5, 6)] = &op_ILL,
        [IDX(0xB5, 7)] = &op_ILL,

        /* 0xB6 LDX zp,Y ; class=r_zpy */
        [IDX(0xB6, 0)] = &mc_r_zpy_c0_addr,
        [IDX(0xB6, 1)] = &mc_r_zpy_c1_idx,
        [IDX(0xB6, 2)] = &mc_r_zpy_c2_data,
        [IDX(0xB6, 3)] = &op_ldx_r_ready_none_pending_data_fetch,
        [IDX(0xB6, 4)] = &mc_dispatch,
        [IDX(0xB6, 5)] = &op_ILL,
        [IDX(0xB6, 6)] = &op_ILL,
        [IDX(0xB6, 7)] = &op_ILL,

        /* 0xB7 undocumented/illegal */
        [IDX(0xB7, 0)] = &op_ILL,
        [IDX(0xB7, 1)] = &op_ILL,
        [IDX(0xB7, 2)] = &op_ILL,
        [IDX(0xB7, 3)] = &op_ILL,
        [IDX(0xB7, 4)] = &op_ILL,
        [IDX(0xB7, 5)] = &op_ILL,
        [IDX(0xB7, 6)] = &op_ILL,
        [IDX(0xB7, 7)] = &op_ILL,

        /* 0xB8 CLV ; class=imp */
        [IDX(0xB8, 0)] = &op_clv_imp_ready_none_pending_none_dummy,
        [IDX(0xB8, 1)] = &mc_fetch,
        [IDX(0xB8, 2)] = &mc_dispatch,
        [IDX(0xB8, 3)] = &op_ILL,
        [IDX(0xB8, 4)] = &op_ILL,
        [IDX(0xB8, 5)] = &op_ILL,
        [IDX(0xB8, 6)] = &op_ILL,
        [IDX(0xB8, 7)] = &op_ILL,

        /* 0xB9 LDA abs,Y ; class=r_aby */
        [IDX(0xB9, 0)] = &mc_r_aby_c0_lo,
        [IDX(0xB9, 1)] = &mc_r_aby_c1_hi,
        [IDX(0xB9, 2)] = &mc_r_aby_c2_probe,
        [IDX(0xB9, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0xB9, 4)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xB9, 5)] = &mc_dispatch,
        [IDX(0xB9, 6)] = &op_ILL,
        [IDX(0xB9, 7)] = &op_ILL,

        /* 0xBA TSX ; class=imp */
        [IDX(0xBA, 0)] = &op_tsx_imp_ready_none_pending_none_dummy,
        [IDX(0xBA, 1)] = &mc_fetch,
        [IDX(0xBA, 2)] = &mc_dispatch,
        [IDX(0xBA, 3)] = &op_ILL,
        [IDX(0xBA, 4)] = &op_ILL,
        [IDX(0xBA, 5)] = &op_ILL,
        [IDX(0xBA, 6)] = &op_ILL,
        [IDX(0xBA, 7)] = &op_ILL,

        /* 0xBB undocumented/illegal */
        [IDX(0xBB, 0)] = &op_ILL,
        [IDX(0xBB, 1)] = &op_ILL,
        [IDX(0xBB, 2)] = &op_ILL,
        [IDX(0xBB, 3)] = &op_ILL,
        [IDX(0xBB, 4)] = &op_ILL,
        [IDX(0xBB, 5)] = &op_ILL,
        [IDX(0xBB, 6)] = &op_ILL,
        [IDX(0xBB, 7)] = &op_ILL,

        /* 0xBC LDY abs,X ; class=r_abx */
        [IDX(0xBC, 0)] = &mc_r_abx_c0_lo,
        [IDX(0xBC, 1)] = &mc_r_abx_c1_hi,
        [IDX(0xBC, 2)] = &mc_r_abx_c2_probe,
        [IDX(0xBC, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0xBC, 4)] = &op_ldy_r_ready_none_pending_data_fetch,
        [IDX(0xBC, 5)] = &mc_dispatch,
        [IDX(0xBC, 6)] = &op_ILL,
        [IDX(0xBC, 7)] = &op_ILL,

        /* 0xBD LDA abs,X ; class=r_abx */
        [IDX(0xBD, 0)] = &mc_r_abx_c0_lo,
        [IDX(0xBD, 1)] = &mc_r_abx_c1_hi,
        [IDX(0xBD, 2)] = &mc_r_abx_c2_probe,
        [IDX(0xBD, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0xBD, 4)] = &op_lda_r_ready_none_pending_data_fetch,
        [IDX(0xBD, 5)] = &mc_dispatch,
        [IDX(0xBD, 6)] = &op_ILL,
        [IDX(0xBD, 7)] = &op_ILL,

        /* 0xBE LDX abs,Y ; class=r_aby */
        [IDX(0xBE, 0)] = &mc_r_aby_c0_lo,
        [IDX(0xBE, 1)] = &mc_r_aby_c1_hi,
        [IDX(0xBE, 2)] = &mc_r_aby_c2_probe,
        [IDX(0xBE, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0xBE, 4)] = &op_ldx_r_ready_none_pending_data_fetch,
        [IDX(0xBE, 5)] = &mc_dispatch,
        [IDX(0xBE, 6)] = &op_ILL,
        [IDX(0xBE, 7)] = &op_ILL,

        /* 0xBF undocumented/illegal */
        [IDX(0xBF, 0)] = &op_ILL,
        [IDX(0xBF, 1)] = &op_ILL,
        [IDX(0xBF, 2)] = &op_ILL,
        [IDX(0xBF, 3)] = &op_ILL,
        [IDX(0xBF, 4)] = &op_ILL,
        [IDX(0xBF, 5)] = &op_ILL,
        [IDX(0xBF, 6)] = &op_ILL,
        [IDX(0xBF, 7)] = &op_ILL,

        /* 0xC0 CPY #imm ; class=r_imm */
        [IDX(0xC0, 0)] = &mc_r_imm_c0_data,
        [IDX(0xC0, 1)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(0xC0, 2)] = &mc_dispatch,
        [IDX(0xC0, 3)] = &op_ILL,
        [IDX(0xC0, 4)] = &op_ILL,
        [IDX(0xC0, 5)] = &op_ILL,
        [IDX(0xC0, 6)] = &op_ILL,
        [IDX(0xC0, 7)] = &op_ILL,

        /* 0xC1 CMP (zp,X) ; class=r_izx */
        [IDX(0xC1, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0xC1, 1)] = &mc_r_izx_c1_idx,
        [IDX(0xC1, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0xC1, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0xC1, 4)] = &mc_r_izx_c4_data,
        [IDX(0xC1, 5)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xC1, 6)] = &mc_dispatch,
        [IDX(0xC1, 7)] = &op_ILL,

        /* 0xC2 undocumented/illegal */
        [IDX(0xC2, 0)] = &op_ILL,
        [IDX(0xC2, 1)] = &op_ILL,
        [IDX(0xC2, 2)] = &op_ILL,
        [IDX(0xC2, 3)] = &op_ILL,
        [IDX(0xC2, 4)] = &op_ILL,
        [IDX(0xC2, 5)] = &op_ILL,
        [IDX(0xC2, 6)] = &op_ILL,
        [IDX(0xC2, 7)] = &op_ILL,

        /* 0xC3 undocumented/illegal */
        [IDX(0xC3, 0)] = &op_ILL,
        [IDX(0xC3, 1)] = &op_ILL,
        [IDX(0xC3, 2)] = &op_ILL,
        [IDX(0xC3, 3)] = &op_ILL,
        [IDX(0xC3, 4)] = &op_ILL,
        [IDX(0xC3, 5)] = &op_ILL,
        [IDX(0xC3, 6)] = &op_ILL,
        [IDX(0xC3, 7)] = &op_ILL,

        /* 0xC4 CPY zp ; class=r_zp */
        [IDX(0xC4, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xC4, 1)] = &mc_r_zp_c1_data,
        [IDX(0xC4, 2)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(0xC4, 3)] = &mc_dispatch,
        [IDX(0xC4, 4)] = &op_ILL,
        [IDX(0xC4, 5)] = &op_ILL,
        [IDX(0xC4, 6)] = &op_ILL,
        [IDX(0xC4, 7)] = &op_ILL,

        /* 0xC5 CMP zp ; class=r_zp */
        [IDX(0xC5, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xC5, 1)] = &mc_r_zp_c1_data,
        [IDX(0xC5, 2)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xC5, 3)] = &mc_dispatch,
        [IDX(0xC5, 4)] = &op_ILL,
        [IDX(0xC5, 5)] = &op_ILL,
        [IDX(0xC5, 6)] = &op_ILL,
        [IDX(0xC5, 7)] = &op_ILL,

        /* 0xC6 DEC zp ; class=rw_zp */
        [IDX(0xC6, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0xC6, 1)] = &mc_rw_zp_c1_data,
        [IDX(0xC6, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0xC6, 3)] = &op_dec_rw_ready_addr_data_pending_none_wr,
        [IDX(0xC6, 4)] = &mc_fetch,
        [IDX(0xC6, 5)] = &mc_dispatch,
        [IDX(0xC6, 6)] = &op_ILL,
        [IDX(0xC6, 7)] = &op_ILL,

        /* 0xC7 undocumented/illegal */
        [IDX(0xC7, 0)] = &op_ILL,
        [IDX(0xC7, 1)] = &op_ILL,
        [IDX(0xC7, 2)] = &op_ILL,
        [IDX(0xC7, 3)] = &op_ILL,
        [IDX(0xC7, 4)] = &op_ILL,
        [IDX(0xC7, 5)] = &op_ILL,
        [IDX(0xC7, 6)] = &op_ILL,
        [IDX(0xC7, 7)] = &op_ILL,

        /* 0xC8 INY ; class=imp */
        [IDX(0xC8, 0)] = &op_iny_imp_ready_none_pending_none_dummy,
        [IDX(0xC8, 1)] = &mc_fetch,
        [IDX(0xC8, 2)] = &mc_dispatch,
        [IDX(0xC8, 3)] = &op_ILL,
        [IDX(0xC8, 4)] = &op_ILL,
        [IDX(0xC8, 5)] = &op_ILL,
        [IDX(0xC8, 6)] = &op_ILL,
        [IDX(0xC8, 7)] = &op_ILL,

        /* 0xC9 CMP #imm ; class=r_imm */
        [IDX(0xC9, 0)] = &mc_r_imm_c0_data,
        [IDX(0xC9, 1)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xC9, 2)] = &mc_dispatch,
        [IDX(0xC9, 3)] = &op_ILL,
        [IDX(0xC9, 4)] = &op_ILL,
        [IDX(0xC9, 5)] = &op_ILL,
        [IDX(0xC9, 6)] = &op_ILL,
        [IDX(0xC9, 7)] = &op_ILL,

        /* 0xCA DEX ; class=imp */
        [IDX(0xCA, 0)] = &op_dex_imp_ready_none_pending_none_dummy,
        [IDX(0xCA, 1)] = &mc_fetch,
        [IDX(0xCA, 2)] = &mc_dispatch,
        [IDX(0xCA, 3)] = &op_ILL,
        [IDX(0xCA, 4)] = &op_ILL,
        [IDX(0xCA, 5)] = &op_ILL,
        [IDX(0xCA, 6)] = &op_ILL,
        [IDX(0xCA, 7)] = &op_ILL,

        /* 0xCB undocumented/illegal */
        [IDX(0xCB, 0)] = &op_ILL,
        [IDX(0xCB, 1)] = &op_ILL,
        [IDX(0xCB, 2)] = &op_ILL,
        [IDX(0xCB, 3)] = &op_ILL,
        [IDX(0xCB, 4)] = &op_ILL,
        [IDX(0xCB, 5)] = &op_ILL,
        [IDX(0xCB, 6)] = &op_ILL,
        [IDX(0xCB, 7)] = &op_ILL,

        /* 0xCC CPY abs ; class=r_abs */
        [IDX(0xCC, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xCC, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xCC, 2)] = &mc_r_abs_c2_data,
        [IDX(0xCC, 3)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(0xCC, 4)] = &mc_dispatch,
        [IDX(0xCC, 5)] = &op_ILL,
        [IDX(0xCC, 6)] = &op_ILL,
        [IDX(0xCC, 7)] = &op_ILL,

        /* 0xCD CMP abs ; class=r_abs */
        [IDX(0xCD, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xCD, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xCD, 2)] = &mc_r_abs_c2_data,
        [IDX(0xCD, 3)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xCD, 4)] = &mc_dispatch,
        [IDX(0xCD, 5)] = &op_ILL,
        [IDX(0xCD, 6)] = &op_ILL,
        [IDX(0xCD, 7)] = &op_ILL,

        /* 0xCE DEC abs ; class=rw_abs */
        [IDX(0xCE, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0xCE, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0xCE, 2)] = &mc_rw_abs_c2_data,
        [IDX(0xCE, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0xCE, 4)] = &op_dec_rw_ready_addr_data_pending_none_wr,
        [IDX(0xCE, 5)] = &mc_fetch,
        [IDX(0xCE, 6)] = &mc_dispatch,
        [IDX(0xCE, 7)] = &op_ILL,

        /* 0xCF undocumented/illegal */
        [IDX(0xCF, 0)] = &op_ILL,
        [IDX(0xCF, 1)] = &op_ILL,
        [IDX(0xCF, 2)] = &op_ILL,
        [IDX(0xCF, 3)] = &op_ILL,
        [IDX(0xCF, 4)] = &op_ILL,
        [IDX(0xCF, 5)] = &op_ILL,
        [IDX(0xCF, 6)] = &op_ILL,
        [IDX(0xCF, 7)] = &op_ILL,

        /* 0xD0 BNE rel ; class=branch_rel */
        [IDX(0xD0, 0)] = &op_bne_branch_c0_offset,
        [IDX(0xD0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0xD0, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0xD0, 3)] = &mc_fetch,
        [IDX(0xD0, 4)] = &mc_dispatch,
        [IDX(0xD0, 5)] = &op_ILL,
        [IDX(0xD0, 6)] = &op_ILL,
        [IDX(0xD0, 7)] = &op_ILL,

        /* 0xD1 CMP (zp),Y ; class=r_izy */
        [IDX(0xD1, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0xD1, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0xD1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0xD1, 3)] = &mc_r_izy_c3_probe,
        [IDX(0xD1, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0xD1, 5)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xD1, 6)] = &mc_dispatch,
        [IDX(0xD1, 7)] = &op_ILL,

        /* 0xD2 undocumented/illegal */
        [IDX(0xD2, 0)] = &op_ILL,
        [IDX(0xD2, 1)] = &op_ILL,
        [IDX(0xD2, 2)] = &op_ILL,
        [IDX(0xD2, 3)] = &op_ILL,
        [IDX(0xD2, 4)] = &op_ILL,
        [IDX(0xD2, 5)] = &op_ILL,
        [IDX(0xD2, 6)] = &op_ILL,
        [IDX(0xD2, 7)] = &op_ILL,

        /* 0xD3 undocumented/illegal */
        [IDX(0xD3, 0)] = &op_ILL,
        [IDX(0xD3, 1)] = &op_ILL,
        [IDX(0xD3, 2)] = &op_ILL,
        [IDX(0xD3, 3)] = &op_ILL,
        [IDX(0xD3, 4)] = &op_ILL,
        [IDX(0xD3, 5)] = &op_ILL,
        [IDX(0xD3, 6)] = &op_ILL,
        [IDX(0xD3, 7)] = &op_ILL,

        /* 0xD4 undocumented/illegal */
        [IDX(0xD4, 0)] = &op_ILL,
        [IDX(0xD4, 1)] = &op_ILL,
        [IDX(0xD4, 2)] = &op_ILL,
        [IDX(0xD4, 3)] = &op_ILL,
        [IDX(0xD4, 4)] = &op_ILL,
        [IDX(0xD4, 5)] = &op_ILL,
        [IDX(0xD4, 6)] = &op_ILL,
        [IDX(0xD4, 7)] = &op_ILL,

        /* 0xD5 CMP zp,X ; class=r_zpx */
        [IDX(0xD5, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0xD5, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0xD5, 2)] = &mc_r_zpx_c2_data,
        [IDX(0xD5, 3)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xD5, 4)] = &mc_dispatch,
        [IDX(0xD5, 5)] = &op_ILL,
        [IDX(0xD5, 6)] = &op_ILL,
        [IDX(0xD5, 7)] = &op_ILL,

        /* 0xD6 DEC zp,X ; class=rw_zpx */
        [IDX(0xD6, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0xD6, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0xD6, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0xD6, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0xD6, 4)] = &op_dec_rw_ready_addr_data_pending_none_wr,
        [IDX(0xD6, 5)] = &mc_fetch,
        [IDX(0xD6, 6)] = &mc_dispatch,
        [IDX(0xD6, 7)] = &op_ILL,

        /* 0xD7 undocumented/illegal */
        [IDX(0xD7, 0)] = &op_ILL,
        [IDX(0xD7, 1)] = &op_ILL,
        [IDX(0xD7, 2)] = &op_ILL,
        [IDX(0xD7, 3)] = &op_ILL,
        [IDX(0xD7, 4)] = &op_ILL,
        [IDX(0xD7, 5)] = &op_ILL,
        [IDX(0xD7, 6)] = &op_ILL,
        [IDX(0xD7, 7)] = &op_ILL,

        /* 0xD8 CLD ; class=imp */
        [IDX(0xD8, 0)] = &op_cld_imp_ready_none_pending_none_dummy,
        [IDX(0xD8, 1)] = &mc_fetch,
        [IDX(0xD8, 2)] = &mc_dispatch,
        [IDX(0xD8, 3)] = &op_ILL,
        [IDX(0xD8, 4)] = &op_ILL,
        [IDX(0xD8, 5)] = &op_ILL,
        [IDX(0xD8, 6)] = &op_ILL,
        [IDX(0xD8, 7)] = &op_ILL,

        /* 0xD9 CMP abs,Y ; class=r_aby */
        [IDX(0xD9, 0)] = &mc_r_aby_c0_lo,
        [IDX(0xD9, 1)] = &mc_r_aby_c1_hi,
        [IDX(0xD9, 2)] = &mc_r_aby_c2_probe,
        [IDX(0xD9, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0xD9, 4)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xD9, 5)] = &mc_dispatch,
        [IDX(0xD9, 6)] = &op_ILL,
        [IDX(0xD9, 7)] = &op_ILL,

        /* 0xDA undocumented/illegal */
        [IDX(0xDA, 0)] = &op_ILL,
        [IDX(0xDA, 1)] = &op_ILL,
        [IDX(0xDA, 2)] = &op_ILL,
        [IDX(0xDA, 3)] = &op_ILL,
        [IDX(0xDA, 4)] = &op_ILL,
        [IDX(0xDA, 5)] = &op_ILL,
        [IDX(0xDA, 6)] = &op_ILL,
        [IDX(0xDA, 7)] = &op_ILL,

        /* 0xDB undocumented/illegal */
        [IDX(0xDB, 0)] = &op_ILL,
        [IDX(0xDB, 1)] = &op_ILL,
        [IDX(0xDB, 2)] = &op_ILL,
        [IDX(0xDB, 3)] = &op_ILL,
        [IDX(0xDB, 4)] = &op_ILL,
        [IDX(0xDB, 5)] = &op_ILL,
        [IDX(0xDB, 6)] = &op_ILL,
        [IDX(0xDB, 7)] = &op_ILL,

        /* 0xDC undocumented/illegal */
        [IDX(0xDC, 0)] = &op_ILL,
        [IDX(0xDC, 1)] = &op_ILL,
        [IDX(0xDC, 2)] = &op_ILL,
        [IDX(0xDC, 3)] = &op_ILL,
        [IDX(0xDC, 4)] = &op_ILL,
        [IDX(0xDC, 5)] = &op_ILL,
        [IDX(0xDC, 6)] = &op_ILL,
        [IDX(0xDC, 7)] = &op_ILL,

        /* 0xDD CMP abs,X ; class=r_abx */
        [IDX(0xDD, 0)] = &mc_r_abx_c0_lo,
        [IDX(0xDD, 1)] = &mc_r_abx_c1_hi,
        [IDX(0xDD, 2)] = &mc_r_abx_c2_probe,
        [IDX(0xDD, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0xDD, 4)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(0xDD, 5)] = &mc_dispatch,
        [IDX(0xDD, 6)] = &op_ILL,
        [IDX(0xDD, 7)] = &op_ILL,

        /* 0xDE DEC abs,X ; class=rw_abx */
        [IDX(0xDE, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0xDE, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0xDE, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0xDE, 3)] = &mc_rw_abx_c3_data,
        [IDX(0xDE, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0xDE, 5)] = &op_dec_rw_ready_addr_data_pending_none_wr,
        [IDX(0xDE, 6)] = &mc_fetch,
        [IDX(0xDE, 7)] = &mc_dispatch,

        /* 0xDF undocumented/illegal */
        [IDX(0xDF, 0)] = &op_ILL,
        [IDX(0xDF, 1)] = &op_ILL,
        [IDX(0xDF, 2)] = &op_ILL,
        [IDX(0xDF, 3)] = &op_ILL,
        [IDX(0xDF, 4)] = &op_ILL,
        [IDX(0xDF, 5)] = &op_ILL,
        [IDX(0xDF, 6)] = &op_ILL,
        [IDX(0xDF, 7)] = &op_ILL,

        /* 0xE0 CPX #imm ; class=r_imm */
        [IDX(0xE0, 0)] = &mc_r_imm_c0_data,
        [IDX(0xE0, 1)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(0xE0, 2)] = &mc_dispatch,
        [IDX(0xE0, 3)] = &op_ILL,
        [IDX(0xE0, 4)] = &op_ILL,
        [IDX(0xE0, 5)] = &op_ILL,
        [IDX(0xE0, 6)] = &op_ILL,
        [IDX(0xE0, 7)] = &op_ILL,

        /* 0xE1 SBC (zp,X) ; class=r_izx */
        [IDX(0xE1, 0)] = &mc_r_izx_c0_ptr,
        [IDX(0xE1, 1)] = &mc_r_izx_c1_idx,
        [IDX(0xE1, 2)] = &mc_r_izx_c2_ptrlo,
        [IDX(0xE1, 3)] = &mc_r_izx_c3_ptrhi,
        [IDX(0xE1, 4)] = &mc_r_izx_c4_data,
        [IDX(0xE1, 5)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xE1, 6)] = &mc_dispatch,
        [IDX(0xE1, 7)] = &op_ILL,

        /* 0xE2 undocumented/illegal */
        [IDX(0xE2, 0)] = &op_ILL,
        [IDX(0xE2, 1)] = &op_ILL,
        [IDX(0xE2, 2)] = &op_ILL,
        [IDX(0xE2, 3)] = &op_ILL,
        [IDX(0xE2, 4)] = &op_ILL,
        [IDX(0xE2, 5)] = &op_ILL,
        [IDX(0xE2, 6)] = &op_ILL,
        [IDX(0xE2, 7)] = &op_ILL,

        /* 0xE3 undocumented/illegal */
        [IDX(0xE3, 0)] = &op_ILL,
        [IDX(0xE3, 1)] = &op_ILL,
        [IDX(0xE3, 2)] = &op_ILL,
        [IDX(0xE3, 3)] = &op_ILL,
        [IDX(0xE3, 4)] = &op_ILL,
        [IDX(0xE3, 5)] = &op_ILL,
        [IDX(0xE3, 6)] = &op_ILL,
        [IDX(0xE3, 7)] = &op_ILL,

        /* 0xE4 CPX zp ; class=r_zp */
        [IDX(0xE4, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xE4, 1)] = &mc_r_zp_c1_data,
        [IDX(0xE4, 2)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(0xE4, 3)] = &mc_dispatch,
        [IDX(0xE4, 4)] = &op_ILL,
        [IDX(0xE4, 5)] = &op_ILL,
        [IDX(0xE4, 6)] = &op_ILL,
        [IDX(0xE4, 7)] = &op_ILL,

        /* 0xE5 SBC zp ; class=r_zp */
        [IDX(0xE5, 0)] = &mc_r_zp_c0_addr,
        [IDX(0xE5, 1)] = &mc_r_zp_c1_data,
        [IDX(0xE5, 2)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xE5, 3)] = &mc_dispatch,
        [IDX(0xE5, 4)] = &op_ILL,
        [IDX(0xE5, 5)] = &op_ILL,
        [IDX(0xE5, 6)] = &op_ILL,
        [IDX(0xE5, 7)] = &op_ILL,

        /* 0xE6 INC zp ; class=rw_zp */
        [IDX(0xE6, 0)] = &mc_rw_zp_c0_addr,
        [IDX(0xE6, 1)] = &mc_rw_zp_c1_data,
        [IDX(0xE6, 2)] = &mc_rw_zp_c2_wb,
        [IDX(0xE6, 3)] = &op_inc_rw_ready_addr_data_pending_none_wr,
        [IDX(0xE6, 4)] = &mc_fetch,
        [IDX(0xE6, 5)] = &mc_dispatch,
        [IDX(0xE6, 6)] = &op_ILL,
        [IDX(0xE6, 7)] = &op_ILL,

        /* 0xE7 undocumented/illegal */
        [IDX(0xE7, 0)] = &op_ILL,
        [IDX(0xE7, 1)] = &op_ILL,
        [IDX(0xE7, 2)] = &op_ILL,
        [IDX(0xE7, 3)] = &op_ILL,
        [IDX(0xE7, 4)] = &op_ILL,
        [IDX(0xE7, 5)] = &op_ILL,
        [IDX(0xE7, 6)] = &op_ILL,
        [IDX(0xE7, 7)] = &op_ILL,

        /* 0xE8 INX ; class=imp */
        [IDX(0xE8, 0)] = &op_inx_imp_ready_none_pending_none_dummy,
        [IDX(0xE8, 1)] = &mc_fetch,
        [IDX(0xE8, 2)] = &mc_dispatch,
        [IDX(0xE8, 3)] = &op_ILL,
        [IDX(0xE8, 4)] = &op_ILL,
        [IDX(0xE8, 5)] = &op_ILL,
        [IDX(0xE8, 6)] = &op_ILL,
        [IDX(0xE8, 7)] = &op_ILL,

        /* 0xE9 SBC #imm ; class=r_imm */
        [IDX(0xE9, 0)] = &mc_r_imm_c0_data,
        [IDX(0xE9, 1)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xE9, 2)] = &mc_dispatch,
        [IDX(0xE9, 3)] = &op_ILL,
        [IDX(0xE9, 4)] = &op_ILL,
        [IDX(0xE9, 5)] = &op_ILL,
        [IDX(0xE9, 6)] = &op_ILL,
        [IDX(0xE9, 7)] = &op_ILL,

        /* 0xEA NOP ; class=imp */
        [IDX(0xEA, 0)] = &op_nop_imp_ready_none_pending_none_dummy,
        [IDX(0xEA, 1)] = &mc_fetch,
        [IDX(0xEA, 2)] = &mc_dispatch,
        [IDX(0xEA, 3)] = &op_ILL,
        [IDX(0xEA, 4)] = &op_ILL,
        [IDX(0xEA, 5)] = &op_ILL,
        [IDX(0xEA, 6)] = &op_ILL,
        [IDX(0xEA, 7)] = &op_ILL,

        /* 0xEB undocumented/illegal */
        [IDX(0xEB, 0)] = &op_ILL,
        [IDX(0xEB, 1)] = &op_ILL,
        [IDX(0xEB, 2)] = &op_ILL,
        [IDX(0xEB, 3)] = &op_ILL,
        [IDX(0xEB, 4)] = &op_ILL,
        [IDX(0xEB, 5)] = &op_ILL,
        [IDX(0xEB, 6)] = &op_ILL,
        [IDX(0xEB, 7)] = &op_ILL,

        /* 0xEC CPX abs ; class=r_abs */
        [IDX(0xEC, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xEC, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xEC, 2)] = &mc_r_abs_c2_data,
        [IDX(0xEC, 3)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(0xEC, 4)] = &mc_dispatch,
        [IDX(0xEC, 5)] = &op_ILL,
        [IDX(0xEC, 6)] = &op_ILL,
        [IDX(0xEC, 7)] = &op_ILL,

        /* 0xED SBC abs ; class=r_abs */
        [IDX(0xED, 0)] = &mc_r_abs_c0_lo,
        [IDX(0xED, 1)] = &mc_r_abs_c1_hi,
        [IDX(0xED, 2)] = &mc_r_abs_c2_data,
        [IDX(0xED, 3)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xED, 4)] = &mc_dispatch,
        [IDX(0xED, 5)] = &op_ILL,
        [IDX(0xED, 6)] = &op_ILL,
        [IDX(0xED, 7)] = &op_ILL,

        /* 0xEE INC abs ; class=rw_abs */
        [IDX(0xEE, 0)] = &mc_rw_abs_c0_lo,
        [IDX(0xEE, 1)] = &mc_rw_abs_c1_hi,
        [IDX(0xEE, 2)] = &mc_rw_abs_c2_data,
        [IDX(0xEE, 3)] = &mc_rw_abs_c3_wb,
        [IDX(0xEE, 4)] = &op_inc_rw_ready_addr_data_pending_none_wr,
        [IDX(0xEE, 5)] = &mc_fetch,
        [IDX(0xEE, 6)] = &mc_dispatch,
        [IDX(0xEE, 7)] = &op_ILL,

        /* 0xEF undocumented/illegal */
        [IDX(0xEF, 0)] = &op_ILL,
        [IDX(0xEF, 1)] = &op_ILL,
        [IDX(0xEF, 2)] = &op_ILL,
        [IDX(0xEF, 3)] = &op_ILL,
        [IDX(0xEF, 4)] = &op_ILL,
        [IDX(0xEF, 5)] = &op_ILL,
        [IDX(0xEF, 6)] = &op_ILL,
        [IDX(0xEF, 7)] = &op_ILL,

        /* 0xF0 BEQ rel ; class=branch_rel */
        [IDX(0xF0, 0)] = &op_beq_branch_c0_offset,
        [IDX(0xF0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(0xF0, 2)] = &mc_branch_rel_c2_pgfix,
        [IDX(0xF0, 3)] = &mc_fetch,
        [IDX(0xF0, 4)] = &mc_dispatch,
        [IDX(0xF0, 5)] = &op_ILL,
        [IDX(0xF0, 6)] = &op_ILL,
        [IDX(0xF0, 7)] = &op_ILL,

        /* 0xF1 SBC (zp),Y ; class=r_izy */
        [IDX(0xF1, 0)] = &mc_r_izy_c0_ptr,
        [IDX(0xF1, 1)] = &mc_r_izy_c1_ptrlo,
        [IDX(0xF1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(0xF1, 3)] = &mc_r_izy_c3_probe,
        [IDX(0xF1, 4)] = &mc_r_izy_c4_pgfix,
        [IDX(0xF1, 5)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xF1, 6)] = &mc_dispatch,
        [IDX(0xF1, 7)] = &op_ILL,

        /* 0xF2 undocumented/illegal */
        [IDX(0xF2, 0)] = &op_ILL,
        [IDX(0xF2, 1)] = &op_ILL,
        [IDX(0xF2, 2)] = &op_ILL,
        [IDX(0xF2, 3)] = &op_ILL,
        [IDX(0xF2, 4)] = &op_ILL,
        [IDX(0xF2, 5)] = &op_ILL,
        [IDX(0xF2, 6)] = &op_ILL,
        [IDX(0xF2, 7)] = &op_ILL,

        /* 0xF3 undocumented/illegal */
        [IDX(0xF3, 0)] = &op_ILL,
        [IDX(0xF3, 1)] = &op_ILL,
        [IDX(0xF3, 2)] = &op_ILL,
        [IDX(0xF3, 3)] = &op_ILL,
        [IDX(0xF3, 4)] = &op_ILL,
        [IDX(0xF3, 5)] = &op_ILL,
        [IDX(0xF3, 6)] = &op_ILL,
        [IDX(0xF3, 7)] = &op_ILL,

        /* 0xF4 undocumented/illegal */
        [IDX(0xF4, 0)] = &op_ILL,
        [IDX(0xF4, 1)] = &op_ILL,
        [IDX(0xF4, 2)] = &op_ILL,
        [IDX(0xF4, 3)] = &op_ILL,
        [IDX(0xF4, 4)] = &op_ILL,
        [IDX(0xF4, 5)] = &op_ILL,
        [IDX(0xF4, 6)] = &op_ILL,
        [IDX(0xF4, 7)] = &op_ILL,

        /* 0xF5 SBC zp,X ; class=r_zpx */
        [IDX(0xF5, 0)] = &mc_r_zpx_c0_addr,
        [IDX(0xF5, 1)] = &mc_r_zpx_c1_idx,
        [IDX(0xF5, 2)] = &mc_r_zpx_c2_data,
        [IDX(0xF5, 3)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xF5, 4)] = &mc_dispatch,
        [IDX(0xF5, 5)] = &op_ILL,
        [IDX(0xF5, 6)] = &op_ILL,
        [IDX(0xF5, 7)] = &op_ILL,

        /* 0xF6 INC zp,X ; class=rw_zpx */
        [IDX(0xF6, 0)] = &mc_rw_zpx_c0_addr,
        [IDX(0xF6, 1)] = &mc_rw_zpx_c1_idx,
        [IDX(0xF6, 2)] = &mc_rw_zpx_c2_data,
        [IDX(0xF6, 3)] = &mc_rw_zpx_c3_wb,
        [IDX(0xF6, 4)] = &op_inc_rw_ready_addr_data_pending_none_wr,
        [IDX(0xF6, 5)] = &mc_fetch,
        [IDX(0xF6, 6)] = &mc_dispatch,
        [IDX(0xF6, 7)] = &op_ILL,

        /* 0xF7 undocumented/illegal */
        [IDX(0xF7, 0)] = &op_ILL,
        [IDX(0xF7, 1)] = &op_ILL,
        [IDX(0xF7, 2)] = &op_ILL,
        [IDX(0xF7, 3)] = &op_ILL,
        [IDX(0xF7, 4)] = &op_ILL,
        [IDX(0xF7, 5)] = &op_ILL,
        [IDX(0xF7, 6)] = &op_ILL,
        [IDX(0xF7, 7)] = &op_ILL,

        /* 0xF8 SED ; class=imp */
        [IDX(0xF8, 0)] = &op_sed_imp_ready_none_pending_none_dummy,
        [IDX(0xF8, 1)] = &mc_fetch,
        [IDX(0xF8, 2)] = &mc_dispatch,
        [IDX(0xF8, 3)] = &op_ILL,
        [IDX(0xF8, 4)] = &op_ILL,
        [IDX(0xF8, 5)] = &op_ILL,
        [IDX(0xF8, 6)] = &op_ILL,
        [IDX(0xF8, 7)] = &op_ILL,

        /* 0xF9 SBC abs,Y ; class=r_aby */
        [IDX(0xF9, 0)] = &mc_r_aby_c0_lo,
        [IDX(0xF9, 1)] = &mc_r_aby_c1_hi,
        [IDX(0xF9, 2)] = &mc_r_aby_c2_probe,
        [IDX(0xF9, 3)] = &mc_r_aby_c3_pgfix,
        [IDX(0xF9, 4)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xF9, 5)] = &mc_dispatch,
        [IDX(0xF9, 6)] = &op_ILL,
        [IDX(0xF9, 7)] = &op_ILL,

        /* 0xFA undocumented/illegal */
        [IDX(0xFA, 0)] = &op_ILL,
        [IDX(0xFA, 1)] = &op_ILL,
        [IDX(0xFA, 2)] = &op_ILL,
        [IDX(0xFA, 3)] = &op_ILL,
        [IDX(0xFA, 4)] = &op_ILL,
        [IDX(0xFA, 5)] = &op_ILL,
        [IDX(0xFA, 6)] = &op_ILL,
        [IDX(0xFA, 7)] = &op_ILL,

        /* 0xFB undocumented/illegal */
        [IDX(0xFB, 0)] = &op_ILL,
        [IDX(0xFB, 1)] = &op_ILL,
        [IDX(0xFB, 2)] = &op_ILL,
        [IDX(0xFB, 3)] = &op_ILL,
        [IDX(0xFB, 4)] = &op_ILL,
        [IDX(0xFB, 5)] = &op_ILL,
        [IDX(0xFB, 6)] = &op_ILL,
        [IDX(0xFB, 7)] = &op_ILL,

        /* 0xFC undocumented/illegal */
        [IDX(0xFC, 0)] = &op_ILL,
        [IDX(0xFC, 1)] = &op_ILL,
        [IDX(0xFC, 2)] = &op_ILL,
        [IDX(0xFC, 3)] = &op_ILL,
        [IDX(0xFC, 4)] = &op_ILL,
        [IDX(0xFC, 5)] = &op_ILL,
        [IDX(0xFC, 6)] = &op_ILL,
        [IDX(0xFC, 7)] = &op_ILL,

        /* 0xFD SBC abs,X ; class=r_abx */
        [IDX(0xFD, 0)] = &mc_r_abx_c0_lo,
        [IDX(0xFD, 1)] = &mc_r_abx_c1_hi,
        [IDX(0xFD, 2)] = &mc_r_abx_c2_probe,
        [IDX(0xFD, 3)] = &mc_r_abx_c3_pgfix,
        [IDX(0xFD, 4)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(0xFD, 5)] = &mc_dispatch,
        [IDX(0xFD, 6)] = &op_ILL,
        [IDX(0xFD, 7)] = &op_ILL,

        /* 0xFE INC abs,X ; class=rw_abx */
        [IDX(0xFE, 0)] = &mc_rw_abx_c0_lo,
        [IDX(0xFE, 1)] = &mc_rw_abx_c1_hi,
        [IDX(0xFE, 2)] = &mc_rw_abx_c2_probe,
        [IDX(0xFE, 3)] = &mc_rw_abx_c3_data,
        [IDX(0xFE, 4)] = &mc_rw_abx_c4_wb,
        [IDX(0xFE, 5)] = &op_inc_rw_ready_addr_data_pending_none_wr,
        [IDX(0xFE, 6)] = &mc_fetch,
        [IDX(0xFE, 7)] = &mc_dispatch,

        /* 0xFF undocumented/illegal */
        [IDX(0xFF, 0)] = &op_ILL,
        [IDX(0xFF, 1)] = &op_ILL,
        [IDX(0xFF, 2)] = &op_ILL,
        [IDX(0xFF, 3)] = &op_ILL,
        [IDX(0xFF, 4)] = &op_ILL,
        [IDX(0xFF, 5)] = &op_ILL,
        [IDX(0xFF, 6)] = &op_ILL,
        [IDX(0xFF, 7)] = &op_ILL,

        /* 0x100 reserved extension row */
        [IDX(0x100, 0)] = &op_ILL,
        [IDX(0x100, 1)] = &op_ILL,
        [IDX(0x100, 2)] = &op_ILL,
        [IDX(0x100, 3)] = &op_ILL,
        [IDX(0x100, 4)] = &op_ILL,
        [IDX(0x100, 5)] = &op_ILL,
        [IDX(0x100, 6)] = &op_ILL,
        [IDX(0x100, 7)] = &op_ILL,

        /* 0x101 reserved extension row */
        [IDX(0x101, 0)] = &op_ILL,
        [IDX(0x101, 1)] = &op_ILL,
        [IDX(0x101, 2)] = &op_ILL,
        [IDX(0x101, 3)] = &op_ILL,
        [IDX(0x101, 4)] = &op_ILL,
        [IDX(0x101, 5)] = &op_ILL,
        [IDX(0x101, 6)] = &op_ILL,
        [IDX(0x101, 7)] = &op_ILL,

        /* 0x102 reserved extension row */
        [IDX(0x102, 0)] = &op_ILL,
        [IDX(0x102, 1)] = &op_ILL,
        [IDX(0x102, 2)] = &op_ILL,
        [IDX(0x102, 3)] = &op_ILL,
        [IDX(0x102, 4)] = &op_ILL,
        [IDX(0x102, 5)] = &op_ILL,
        [IDX(0x102, 6)] = &op_ILL,
        [IDX(0x102, 7)] = &op_ILL,

        /* 0x103 reserved extension row */
        [IDX(0x103, 0)] = &op_ILL,
        [IDX(0x103, 1)] = &op_ILL,
        [IDX(0x103, 2)] = &op_ILL,
        [IDX(0x103, 3)] = &op_ILL,
        [IDX(0x103, 4)] = &op_ILL,
        [IDX(0x103, 5)] = &op_ILL,
        [IDX(0x103, 6)] = &op_ILL,
        [IDX(0x103, 7)] = &op_ILL,

        /* 0x104 reserved extension row */
        [IDX(0x104, 0)] = &op_ILL,
        [IDX(0x104, 1)] = &op_ILL,
        [IDX(0x104, 2)] = &op_ILL,
        [IDX(0x104, 3)] = &op_ILL,
        [IDX(0x104, 4)] = &op_ILL,
        [IDX(0x104, 5)] = &op_ILL,
        [IDX(0x104, 6)] = &op_ILL,
        [IDX(0x104, 7)] = &op_ILL,

        /* 0x105 reserved extension row */
        [IDX(0x105, 0)] = &op_ILL,
        [IDX(0x105, 1)] = &op_ILL,
        [IDX(0x105, 2)] = &op_ILL,
        [IDX(0x105, 3)] = &op_ILL,
        [IDX(0x105, 4)] = &op_ILL,
        [IDX(0x105, 5)] = &op_ILL,
        [IDX(0x105, 6)] = &op_ILL,
        [IDX(0x105, 7)] = &op_ILL,

        /* 0x106 reserved extension row */
        [IDX(0x106, 0)] = &op_ILL,
        [IDX(0x106, 1)] = &op_ILL,
        [IDX(0x106, 2)] = &op_ILL,
        [IDX(0x106, 3)] = &op_ILL,
        [IDX(0x106, 4)] = &op_ILL,
        [IDX(0x106, 5)] = &op_ILL,
        [IDX(0x106, 6)] = &op_ILL,
        [IDX(0x106, 7)] = &op_ILL,

        /* 0x107 reserved extension row */
        [IDX(0x107, 0)] = &op_ILL,
        [IDX(0x107, 1)] = &op_ILL,
        [IDX(0x107, 2)] = &op_ILL,
        [IDX(0x107, 3)] = &op_ILL,
        [IDX(0x107, 4)] = &op_ILL,
        [IDX(0x107, 5)] = &op_ILL,
        [IDX(0x107, 6)] = &op_ILL,
        [IDX(0x107, 7)] = &op_ILL,

        /* 0x108 reserved extension row */
        [IDX(0x108, 0)] = &op_ILL,
        [IDX(0x108, 1)] = &op_ILL,
        [IDX(0x108, 2)] = &op_ILL,
        [IDX(0x108, 3)] = &op_ILL,
        [IDX(0x108, 4)] = &op_ILL,
        [IDX(0x108, 5)] = &op_ILL,
        [IDX(0x108, 6)] = &op_ILL,
        [IDX(0x108, 7)] = &op_ILL,

        /* 0x109 reserved extension row */
        [IDX(0x109, 0)] = &op_ILL,
        [IDX(0x109, 1)] = &op_ILL,
        [IDX(0x109, 2)] = &op_ILL,
        [IDX(0x109, 3)] = &op_ILL,
        [IDX(0x109, 4)] = &op_ILL,
        [IDX(0x109, 5)] = &op_ILL,
        [IDX(0x109, 6)] = &op_ILL,
        [IDX(0x109, 7)] = &op_ILL,

        /* 0x10A reserved extension row */
        [IDX(0x10A, 0)] = &op_ILL,
        [IDX(0x10A, 1)] = &op_ILL,
        [IDX(0x10A, 2)] = &op_ILL,
        [IDX(0x10A, 3)] = &op_ILL,
        [IDX(0x10A, 4)] = &op_ILL,
        [IDX(0x10A, 5)] = &op_ILL,
        [IDX(0x10A, 6)] = &op_ILL,
        [IDX(0x10A, 7)] = &op_ILL,

        /* 0x10B reserved extension row */
        [IDX(0x10B, 0)] = &op_ILL,
        [IDX(0x10B, 1)] = &op_ILL,
        [IDX(0x10B, 2)] = &op_ILL,
        [IDX(0x10B, 3)] = &op_ILL,
        [IDX(0x10B, 4)] = &op_ILL,
        [IDX(0x10B, 5)] = &op_ILL,
        [IDX(0x10B, 6)] = &op_ILL,
        [IDX(0x10B, 7)] = &op_ILL,

        /* 0x10C reserved extension row */
        [IDX(0x10C, 0)] = &op_ILL,
        [IDX(0x10C, 1)] = &op_ILL,
        [IDX(0x10C, 2)] = &op_ILL,
        [IDX(0x10C, 3)] = &op_ILL,
        [IDX(0x10C, 4)] = &op_ILL,
        [IDX(0x10C, 5)] = &op_ILL,
        [IDX(0x10C, 6)] = &op_ILL,
        [IDX(0x10C, 7)] = &op_ILL,

        /* 0x10D reserved extension row */
        [IDX(0x10D, 0)] = &op_ILL,
        [IDX(0x10D, 1)] = &op_ILL,
        [IDX(0x10D, 2)] = &op_ILL,
        [IDX(0x10D, 3)] = &op_ILL,
        [IDX(0x10D, 4)] = &op_ILL,
        [IDX(0x10D, 5)] = &op_ILL,
        [IDX(0x10D, 6)] = &op_ILL,
        [IDX(0x10D, 7)] = &op_ILL,

        /* 0x10E reserved extension row */
        [IDX(0x10E, 0)] = &op_ILL,
        [IDX(0x10E, 1)] = &op_ILL,
        [IDX(0x10E, 2)] = &op_ILL,
        [IDX(0x10E, 3)] = &op_ILL,
        [IDX(0x10E, 4)] = &op_ILL,
        [IDX(0x10E, 5)] = &op_ILL,
        [IDX(0x10E, 6)] = &op_ILL,
        [IDX(0x10E, 7)] = &op_ILL,

        /* 0x10F reserved extension row */
        [IDX(0x10F, 0)] = &op_ILL,
        [IDX(0x10F, 1)] = &op_ILL,
        [IDX(0x10F, 2)] = &op_ILL,
        [IDX(0x10F, 3)] = &op_ILL,
        [IDX(0x10F, 4)] = &op_ILL,
        [IDX(0x10F, 5)] = &op_ILL,
        [IDX(0x10F, 6)] = &op_ILL,
        [IDX(0x10F, 7)] = &op_ILL,

};

#undef IDX
