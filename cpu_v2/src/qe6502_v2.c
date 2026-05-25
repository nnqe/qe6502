#include "qe6502.h"
#include <stdbool.h>

static const uint8_t flag_C  = qe6502_flag_C ;
static const uint8_t flag_Z  = qe6502_flag_Z ;
static const uint8_t flag_I  = qe6502_flag_I ;
static const uint8_t flag_D  = qe6502_flag_D ;
static const uint8_t flag_B  = qe6502_flag_B ;
static const uint8_t flag_UN = qe6502_flag_UN;
static const uint8_t flag_V  = qe6502_flag_V ;
static const uint8_t flag_N  = qe6502_flag_N ;

static inline uint8_t flag_C_if(bool cond) { return cond ? flag_C : 0; }
static inline uint8_t flag_Z_if(bool cond) { return cond ? flag_Z : 0; }
static inline uint8_t flag_V_if(bool cond) { return cond ? flag_V : 0; }

static inline uint8_t u16_get_byte(uint16_t x, unsigned byte_index)
{
    return (uint8_t)(x >> (byte_index * 8));
}

static inline uint16_t u16_set_byte(uint16_t x, unsigned byte_index, uint8_t value)
{
    unsigned shift = byte_index * 8;
    uint16_t mask = (uint16_t)((uint16_t)0xFFu << shift);
    return (uint16_t)((x & (uint16_t)~mask) | ((uint16_t)value << shift));
}

enum
{
    opcode_slot_count = 256u,
    service_slot_base = opcode_slot_count,
    service_slot_count = qe6502_slots_per_model - opcode_slot_count
};
QE6502_STATIC_ASSERT((opcode_slot_count & (opcode_slot_count - 1u)) == 0u,
                     "opcode_slot_count must be a power of two");
QE6502_STATIC_ASSERT(opcode_slot_count == 256u,
                     "opcode slots must cover the 8-bit opcode space");
QE6502_STATIC_ASSERT(service_slot_base == opcode_slot_count,
                     "service slots must follow opcode slots");
QE6502_STATIC_ASSERT((service_slot_count + opcode_slot_count) == qe6502_slots_per_model,
                     "opcode and service slot ranges must cover one model block");

typedef enum service_slot
{
    service_slot_reset = 0,
    service_slot_light_reset = 1,
    service_slot_goto = 2,
    service_slot_nmi = 3,
    service_slot_irq = 4,

    service_slot_count_used
} service_slot_t;

QE6502_STATIC_ASSERT((unsigned int)service_slot_count_used <= service_slot_count,
                 "service slot index space overflow");

#define IDX(model, slot, cycle) ((((model) & 0x0Fu) << 12u) + (((slot) & 0x1FFu) << 3u) + (cycle))
#define SERVICE_SLOT_IDX(model, service, cycle) IDX((model), (service_slot_base + (service)), (cycle))

static inline void enter_service_slot(qe6502_t* cpu, service_slot_t slot)
{
    cpu->microcode = (uint16_t)SERVICE_SLOT_IDX(cpu->model, slot, 0u);
}

static inline uint8_t current_opcode_slot(const qe6502_t* cpu)
{
    return (uint8_t)((cpu->microcode >> qe6502_microcode_index_bits) & 0x00ffu);
}

static inline void set_latch_addr0(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_addr, 0, value);
}

static inline void set_latch_addr1(qe6502_t* cpu, uint8_t value)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_addr, 1, value);
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

static inline qe6502_tick_t stack_read(qe6502_t* cpu)
{
    cpu->S++;
    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

qe6502_tick_t qe6502_goto(qe6502_t *cpu, uint16_t address)
{
    cpu->status = 0;
    cpu->PC = address;
    enter_service_slot(cpu, service_slot_goto);
    return qe6502_tick(cpu, 0u);
}

qe6502_tick_t qe6502_reset(qe6502_t *cpu)
{
    cpu->status = 0;
    enter_service_slot(cpu, service_slot_light_reset);
    return qe6502_tick(cpu, 0u);
}

qe6502_tick_t dispatcher(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)(((uint16_t)cpu->model << 12u) | ((uint16_t)bus << 3u));
    cpu->PC++;
    return qe6502_control_store[cpu->microcode](cpu, bus);
}

static inline void update_flags_nz(qe6502_t* cpu, uint8_t value)
{
    const uint8_t mask = (uint8_t)(flag_Z | flag_N);
    const uint8_t flags = (uint8_t)(flag_Z_if(value == 0u) | (value & flag_N));

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | flags);
}

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

static inline uint8_t asl_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t result = (uint8_t)(value << 1);
    update_flags_nzc(cpu, result, flag_C_if((value & flag_N) != 0u));
    return result;
}

static inline uint8_t lsr_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t result = (uint8_t)(value >> 1);
    update_flags_nzc(cpu, result, flag_C_if((value & 1u) != 0u));
    return result;
}

static inline uint8_t rol_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t old_carry = (uint8_t)(cpu->P & flag_C);
    const uint8_t result = (uint8_t)((value << 1) | old_carry);
    update_flags_nzc(cpu, result, flag_C_if((value & flag_N) != 0u));
    return result;
}

static inline uint8_t ror_value(qe6502_t* cpu, uint8_t value)
{
    const uint8_t old_carry = (uint8_t)(cpu->P & flag_C);
    const uint8_t result = (uint8_t)((value >> 1) | (uint8_t)(old_carry << 7));
    update_flags_nzc(cpu, result, flag_C_if((value & 1u) != 0u));
    return result;
}

static inline void update_flags_cvzn(qe6502_t* cpu, uint8_t flags)
{
    const uint8_t mask = (uint8_t)(flag_C | flag_Z | flag_V | flag_N);
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~mask)) | (flags & mask));
}

static inline void adc_binary(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint16_t result16 = (uint16_t)((uint16_t)cpu->A + (uint16_t)value + (uint16_t)carry);
    const uint8_t result = (uint8_t)result16;

    const uint8_t flags = (uint8_t)(
        flag_C_if(result16 > 0xffu) |
        flag_Z_if(result == 0u) |
        (result & flag_N) |
        flag_V_if((((uint8_t)(~(cpu->A ^ value)) & (cpu->A ^ result)) & flag_N) != 0u)
        );

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void adc_decimal_nmos(qe6502_t* cpu, uint8_t value)
{
    uint8_t flags = 0;

    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint8_t bin_result = (uint8_t)(cpu->A + value + carry);
    flags |= flag_Z_if(bin_result == 0u);

    uint8_t low = (uint8_t)((cpu->A & 0x0fu) + (value & 0x0fu) + carry);
    uint8_t high = (uint8_t)((cpu->A >> 4) + (value >> 4));
    if (low > 9u)
    {
        low = (uint8_t)(low - 10u);
        high++;
    }

    uint8_t result = (uint8_t)(((unsigned int)(uint8_t)high << 4u) |
                               ((unsigned int)(uint8_t)low & 0x0fu));
    flags |= (uint8_t)(result & flag_N);
    if ((((uint8_t)(~(cpu->A ^ value)) & (cpu->A ^ result)) & flag_N) != 0u)
    {
        flags |= flag_V;
    }

    if (high > 9u)
    {
        result = (uint8_t)(result - (10u * 16u));
        flags |= flag_C;
    }

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void adc_value(qe6502_t* cpu, uint8_t value)
{
    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, value);
    }
    else
    {
        adc_decimal_nmos(cpu, value);
    }
}

static inline void sbc_decimal_nmos(qe6502_t* cpu, uint8_t value)
{
    uint8_t flags = flag_C;

    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    const uint8_t bin_result = (uint8_t)(cpu->A + (uint8_t)(value ^ 0xffu) + carry);
    flags |= flag_Z_if(bin_result == 0u);

    const int8_t carry_inv = (carry == 0u) ? 1 : 0;
    int8_t low = (int8_t)((int8_t)(cpu->A & 0x0fu) - (int8_t)(value & 0x0fu) - carry_inv);
    int8_t high = (int8_t)((int8_t)(cpu->A >> 4) - (int8_t)(value >> 4));

    if (low < 0)
    {
        low = (int8_t)(low + 10);
        high = (int8_t)(high - 1);
    }

    uint8_t result = (uint8_t)(((unsigned int)(uint8_t)high << 4u) |
                               ((unsigned int)(uint8_t)low & 0x0fu));
    flags |= (uint8_t)(result & flag_N);
    if ((((cpu->A ^ value) & (cpu->A ^ result)) & flag_N) != 0u)
    {
        flags |= flag_V;
    }

    if (high < 0)
    {
        result = (uint8_t)(result + (10u * 16u));
        flags = (uint8_t)(flags & (uint8_t)(~flag_C));
    }

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}

static inline void sbc_value(qe6502_t* cpu, uint8_t value)
{
    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(value ^ 0xffu));
    }
    else
    {
        sbc_decimal_nmos(cpu, value);
    }
}

static inline void adc_decimal_cmos(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    uint16_t result = (uint16_t)((cpu->A & 0x0fu) + (value & 0x0fu) + carry);
    uint8_t flags = 0u;

    if (result >= 0x0au)
    {
        result = (uint16_t)(0x10u | ((result + 6u) & 0x0fu));
    }

    result = (uint16_t)(result + (cpu->A & 0xf0u) + (value & 0xf0u));
    if (result >= 0xa0u)
    {
        flags = (uint8_t)(flags | flag_C);
        flags = (uint8_t)(flags | flag_V_if((result < 0x180u) && (((cpu->A ^ value) & flag_N) == 0u)));
        result = (uint16_t)(result + 0x60u);
    }
    else
    {
        flags = (uint8_t)(flags | flag_V_if((result >= 0x80u) && (((cpu->A ^ value) & flag_N) == 0u)));
    }

    const uint8_t result8 = (uint8_t)result;
    flags = (uint8_t)(flags | flag_Z_if(result8 == 0u) | (result8 & flag_N));

    update_flags_cvzn(cpu, flags);
    cpu->A = result8;
}

static inline void sbc_decimal_cmos(qe6502_t* cpu, uint8_t value)
{
    const uint8_t carry = ((cpu->P & flag_C) != 0u) ? 1u : 0u;
    uint8_t low = (uint8_t)(0x0fu + (cpu->A & 0x0fu) - (value & 0x0fu) + carry);
    uint16_t high = (uint16_t)((uint16_t)(cpu->A & 0xf0u) - (uint16_t)(value & 0xf0u));
    uint8_t flags = 0u;

    if (low < 0x10u)
    {
        low = (uint8_t)(low - 0x06u);
    }
    else
    {
        high = (uint16_t)(high + 0x10u);
        low = (uint8_t)(low - 0x10u);
    }

    const uint8_t overflow_probe = (uint8_t)((high & 0xf0u) + (low & 0x0fu) - 0x10u);
    if ((((cpu->A ^ value) & (cpu->A ^ overflow_probe)) & flag_N) != 0u)
    {
        flags = (uint8_t)(flags | flag_V);
    }

    high = (uint16_t)(high + 0xf0u);
    if (high < 0x100u)
    {
        high = (uint16_t)(high - 0x60u);
    }
    else
    {
        flags = (uint8_t)(flags | flag_C);
    }

    const uint8_t result = (uint8_t)(high + low);
    flags = (uint8_t)(flags | flag_Z_if(result == 0u) | (result & flag_N));

    update_flags_cvzn(cpu, flags);
    cpu->A = result;
}


/* Microcode handlers. */

/* illegal_handler; role=illegal; action=trap_on_cpu_error */
static qe6502_tick_t op_error_trap(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    cpu->status = (uint8_t)(qe6502_status_trapped);
    cpu->microcode--;
    return read(cpu, cpu->PC);
}

/* shared_handler; role=dispatch; action=consume_fetched_opcode_and_dispatch_to_opcode_class */
static qe6502_tick_t mc_dispatch(qe6502_t* cpu, uint8_t bus)
{
    cpu->microcode = (uint16_t)(((uint16_t)cpu->model << 12u) | ((uint16_t)bus << 3u));
    cpu->PC++;
    return qe6502_control_store[cpu->microcode](cpu, bus);
}

/* shared_handler; role=fetch; action=consume_previous_bus_cycle_and_fetch_next_opcode */
static qe6502_tick_t mc_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = (uint8_t)(tick.status | qe6502_status_opcode_fetch);
    return tick;
}

static qe6502_tick_t
read_pc_inc(qe6502_t* cpu)
{
    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* shared_handler; role=read_pc_inc; action=read_pc_and_increment_pc */
static qe6502_tick_t mc_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read_pc_inc(cpu);
}

/* shared_handler; role=read_pc; action=read_pc_without_incrementing_pc */
static qe6502_tick_t mc_read_pc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->PC);
}


/* shared_handler; role=clear_latch_addr_read_pc_inc; action=clear_effective_address_then_read_pc_and_increment_pc */
static qe6502_tick_t mc_clear_latch_addr_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    return read_pc_inc(cpu);
}

/* shared_handler; role=clear_latch_addr_read_pc; action=clear_effective_address_then_read_pc_without_incrementing_pc */
static qe6502_tick_t mc_clear_latch_addr_read_pc(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = 0u;

    return read(cpu, cpu->PC);
}

/* shared_handler; role=read_latch_addr; action=read_effective_address_from_latch_addr */
static qe6502_tick_t mc_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, cpu->latch_addr);
}

/* shared_handler; role=latch_data_write_latch_addr; action=latch_bus_data_then_write_effective_address */
static qe6502_tick_t mc_latch_data_write_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return write(cpu, cpu->latch_addr, cpu->latch_data);
}


/* shared_handler; role=read_stack_current_s; action=read_stack_address_without_incrementing_s */
static qe6502_tick_t mc_read_stack_current_s(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* common_handler; action=ignore_bus_increment_s_and_read_stack */
static qe6502_tick_t mc_stack_pull_read(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_read(cpu);
}

/* shared_handler; role=fetch; action=consume_vector_high_and_fetch_next_opcode_without_interrupt_check */
static qe6502_tick_t mc_fetch_no_interrupts(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);

    qe6502_tick_t tick = read(cpu, cpu->PC);
    tick.status = (uint8_t)(tick.status | qe6502_status_opcode_fetch);
    return tick;
}

/* service_handler; role=vec_lo; action=read_reset_vector_low */
static qe6502_tick_t mc_light_reset_c0_vec_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return read(cpu, 0xfffcu);
}

/* service_handler; role=vec_hi; action=consume_reset_vector_low_and_read_reset_vector_high */
static qe6502_tick_t mc_light_reset_c1_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);

    return read(cpu, 0xfffdu);
}

/* common_handler; role=latch_pch_fetch; action=consume_pc_high_and_fetch_opcode */
static qe6502_tick_t mc_latch_pch_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);

    return mc_fetch(cpu, bus);
}

/* control_handler; role=apply; action=apply_signed_offset_to_pc_low_and_request_branch_dummy_read */
static inline qe6502_tick_t mc_branch_rel_c1_apply(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t old_pc = cpu->PC;
    const int8_t offset = (int8_t)bus;
    const uint16_t target = (uint16_t)(old_pc + offset);

    qe6502_tick_t tick = read(cpu, old_pc);
    cpu->PC = u16_set_byte(cpu->PC, 0, u16_get_byte(target, 0));

    if (u16_get_byte(old_pc, 1) == u16_get_byte(target, 1))
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

/* control_handler; model=wdc; role=offset_or_skip; action=read_bbr_bbs_offset_when_selected_bit_matches_or_dummy_read_it_when_not_taken */
static qe6502_tick_t mc_wdc_bbr_bbs_c3_offset_or_skip(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t opcode = current_opcode_slot(cpu);
    uint8_t bit = (uint8_t)(opcode >> 4u);
    bool taken;

    cpu->latch_data = bus;

    if (bit < 8u)
    {
        const uint8_t mask = (uint8_t)(1u << bit);
        taken = (bus & mask) == 0u;
    }
    else
    {
        bit = (uint8_t)(bit - 8u);
        const uint8_t mask = (uint8_t)(1u << bit);
        taken = (bus & mask) != 0u;
    }

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;

    if (!taken)
    {
        cpu->microcode = (uint16_t)(cpu->microcode + 2u);
    }

    return tick;
}

/* control_handler; model=wdc; role=apply; action=apply_bbr_bbs_signed_offset_and_dummy_read_branch_base_address */
static qe6502_tick_t mc_wdc_bbr_bbs_c4_apply(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t old_pc = cpu->PC;
    const int8_t offset = (int8_t)bus;
    const uint16_t target = (uint16_t)(old_pc + offset);

    qe6502_tick_t tick = read(cpu, old_pc);

    if (u16_get_byte(old_pc, 1) == u16_get_byte(target, 1))
    {
        cpu->PC = target;
        cpu->microcode++;
    }
    else
    {
        cpu->latch_addr = target;
    }

    return tick;
}

/* control_handler; model=wdc; role=pgfix; action=perform_bbr_bbs_page_cross_dummy_read_and_commit_target_pc */
static qe6502_tick_t mc_wdc_bbr_bbs_c5_pgfix(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC = cpu->latch_addr;
    return tick;
}

/* common_handler; role=push_pc_high; action=push_pc_high_to_stack */
static qe6502_tick_t mc_stack_push_pc_high(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, u16_get_byte(cpu->PC, 1));
}

/* common_handler; role=push_pc_low; action=push_pc_low_to_stack */
static qe6502_tick_t mc_stack_push_pc_low(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, u16_get_byte(cpu->PC, 0));
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
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)(cpu->P | flag_I);

    return read(cpu, 0xffffu);
}

/* special_handler; model=wdc; role=vec_hi; action=read_brk_vector_high_to_pc_high_set_interrupt_disable_and_clear_decimal */
static qe6502_tick_t mc_wdc_brk_c5_vec_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);
    cpu->P = (uint8_t)(cpu->P | flag_I);
    cpu->P = (uint8_t)(cpu->P & (uint8_t)(~flag_D));

    return read(cpu, 0xffffu);
}
/* common_handler; action=latch_bus_as_addr_low_read_pc_and_increment_pc */
static inline qe6502_tick_t mc_latch_addr0_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read_pc_inc(cpu);
}

/* common_handler; role=latch_addr1_set_pc_fetch; action=set_effective_address_high_set_pc_and_fetch_next_opcode */
static inline qe6502_tick_t mc_latch_addr1_set_pc_fetch(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    cpu->PC = cpu->latch_addr;

    return mc_fetch(cpu, bus);
}

/* special_handler; role=target_lo; action=read_pointer_to_ea_low_then_increment_pointer_low_with_nmos_wrap */
static inline qe6502_tick_t mc_jmp_ind_c2_target_lo(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(u16_get_byte(cpu->latch_addr, 0) + 1u));
    return tick;
}

/* special_handler; model=wdc; role=target_lo; action=read_pointer_to_ea_low_then_increment_pointer_low */
static inline qe6502_tick_t mc_wdc_jmp_ind_c2_target_lo(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    set_latch_addr0(cpu, (uint8_t)(u16_get_byte(cpu->latch_addr, 0) + 1u));
    return tick;
}

/* special_handler; model=wdc; role=target_hi_retry; action=fix_indirect_pointer_page_cross_and_read_target_high_again */
static inline qe6502_tick_t mc_wdc_jmp_ind_c4_target_hi_retry(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    if (u16_get_byte(cpu->latch_addr, 0) == 0u)
    {
        set_latch_addr1(cpu, (uint8_t)(u16_get_byte(cpu->latch_addr, 1) + 1u));
    }

    return read(cpu, cpu->latch_addr);
}

/* common_handler; action=latch_bus_as_data_read_latch_addr */
static inline qe6502_tick_t mc_latch_data_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return read(cpu, cpu->latch_addr);
}

/* special_handler; role=fetch; action=set_pc_to_effective_address_and_fetch_next_opcode */
static inline qe6502_tick_t mc_jmp_ind_c4_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->latch_data, 1, bus);

    return mc_fetch(cpu, bus);
}

/* special_handler; model=wdc; role=dummy; action=latch_absolute_base_high_and_dummy_read_low_operand_address */
static inline qe6502_tick_t mc_wdc_jmp_ind_x_c2_dummy(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    return read(cpu, (uint16_t)(cpu->PC - 2u));
}

/* special_handler; model=wdc; role=target_lo; action=add_x_to_absolute_base_read_target_low_then_increment_pointer */
static inline qe6502_tick_t mc_wdc_jmp_ind_x_c3_target_lo(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_addr = (uint16_t)(cpu->latch_addr + cpu->X);
    qe6502_tick_t tick = read(cpu, cpu->latch_addr);
    cpu->latch_addr++;
    return tick;
}

/* special_handler; model=wdc; role=target_hi; action=latch_target_low_and_read_target_high */
static inline qe6502_tick_t mc_wdc_jmp_ind_x_c4_target_hi(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    return read(cpu, cpu->latch_addr);
}

/* special_handler; role=dummy; action=dummy_stack_read_before_pushes */
static inline qe6502_tick_t mc_jsr_abs_c1_dummy(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read(cpu, (uint16_t)(0x0100u | cpu->S));
}

/* common_handler; action=latch_bus_as_addr_high_read_latch_addr */
static inline qe6502_tick_t mc_latch_addr1_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* common_handler; role=latch_abx_base_read_pc_inc; action=read_pc_to_ea_high_increment_pc_add_x_low_and_record_page_cross */
static inline qe6502_tick_t mc_latch_abx_base_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read_pc_inc(cpu);
}

/* addressing_handler; model=wdc; role=latch_abx_base_read_pc; action=read_pc_to_ea_high_add_x_low_record_page_cross_without_incrementing_pc */
static inline qe6502_tick_t mc_wdc_latch_abx_base_read_pc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00ffu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, cpu->PC);
}

/* common_handler; role=r_indexed_probe; action=read_indexed_probe_address_to_data_and_skip_pgfix_if_no_page_cross */
static inline qe6502_tick_t mc_r_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 1, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    if (page_cross == 0u)
    {
        cpu->microcode++;
    }

    return tick;
}

/* common_handler; role=latch_aby_base_read_pc_inc; action=read_pc_to_ea_high_increment_pc_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_latch_aby_base_read_pc_inc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read_pc_inc(cpu);
}

/* addressing_handler; model=wdc; role=latch_aby_base_read_pc; action=read_pc_to_ea_high_add_y_low_record_page_cross_without_incrementing_pc */
static inline qe6502_tick_t mc_wdc_latch_aby_base_read_pc(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00ffu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, cpu->PC);
}

/* addressing_handler; model=wdc; role=r_indexed_probe; action=read_cmos_indexed_data_or_pc_dummy_and_increment_pc */
static inline qe6502_tick_t mc_wdc_r_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
    if (page_cross == 0u)
    {
        cpu->PC++;
        cpu->microcode++;
        return read(cpu, cpu->latch_addr);
    }

    qe6502_tick_t tick = read(cpu, cpu->PC);
    cpu->PC++;
    return tick;
}

/* addressing_handler; model=wdc; role=w_indexed_probe; action=dummy_read_pc_increment_pc_and_fix_effective_address_high */
static inline qe6502_tick_t mc_wdc_w_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));
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

/* common_handler; role=izx_read_ptrlo_inc_ptr; action=read_zp_pointer_to_ea_low_then_increment_pointer_wraparound */
static inline qe6502_tick_t mc_izx_read_ptrlo_inc_ptr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint16_t addr = cpu->latch_addr;
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(u16_get_byte(addr, 0) + 1u));
    return tick;
}

/* addressing_handler; model=wdc; role=ptrlo; action=read_zp_pointer_to_ea_low */
static inline qe6502_tick_t mc_izp_c1_ptrlo(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; model=wdc; role=ptrhi; action=latch_effective_address_low_then_read_zp_pointer_high_wraparound */
static inline qe6502_tick_t mc_izp_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t ptr = u16_get_byte(cpu->latch_addr, 0);
    const uint16_t ptr_hi_addr = (uint8_t)(ptr + 1u);

    set_latch_addr0(cpu, bus);

    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=data; action=read_effective_address_to_data */
static inline qe6502_tick_t mc_r_izx_c4_data(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = u16_set_byte(cpu->latch_data, 1, bus);

    return read(cpu, cpu->latch_addr);
}

/* common_handler; action=latch_bus_as_addr_low_read_latch_addr */
static inline qe6502_tick_t mc_latch_addr0_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr0(cpu, bus);

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=ptrhi; action=increment_pointer_read_ea_high_add_y_low_and_record_page_cross */
static inline qe6502_tick_t mc_r_izy_c2_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);
    const uint16_t ptr_hi_addr = u16_set_byte(
        cpu->latch_addr,
        0,
        (uint8_t)(u16_get_byte(cpu->latch_addr, 0) + 1u)
        );

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, ptr_hi_addr);
}

/* addressing_handler; role=data; action=read_zero_page_address_to_data */
static qe6502_tick_t mc_r_zp_c1_data(qe6502_t* cpu, uint8_t bus)
{
    return read(cpu, bus);
}

/* addressing_handler; role=data; action=latch_zero_page_effective_address_and_read_data */
static qe6502_tick_t mc_latch_zp_addr_read_latch_addr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;

    return read(cpu, cpu->latch_addr);
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_r_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));

    return tick;
}

/* addressing_handler; role=idx; action=dummy_read_zero_page_base_then_add_y_wraparound */
static qe6502_tick_t mc_r_zpy_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->Y));

    return tick;
}

/* special_handler; role=pull_pcl; action=normalize_status_increment_s_and_read_pc_low */
static qe6502_tick_t mc_rti_c3_pull_pcl(qe6502_t* cpu, uint8_t bus)
{
    cpu->P = (uint8_t)((bus | flag_UN) & (uint8_t)(~flag_B));

    return stack_read(cpu);
}

/* common_handler; role=latch_pcl_stack_read; action=consume_pc_low_increment_s_and_read_pc_high */
static qe6502_tick_t mc_latch_pcl_stack_read(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 0, bus);

    return stack_read(cpu);
}

/* special_handler; role=inc_pc_dummy; action=dummy_read_return_address_then_increment_pc */
static qe6502_tick_t mc_rts_c4_inc_pc_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->PC = u16_set_byte(cpu->PC, 1, bus);

    return read_pc_inc(cpu);
}

/* rw_abx c1: capture low byte, add X to low byte, request high operand byte */
static inline qe6502_tick_t mc_rw_abx_c1_hi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t indexed = (uint16_t)((uint16_t)bus + cpu->X);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read_pc_inc(cpu);
}

/* rw_zpx c1: capture base address, dummy-read it, then add X with zero-page wraparound */
static inline qe6502_tick_t mc_rw_zpx_c1_idx(qe6502_t* cpu, uint8_t bus)
{
    uint16_t addr = u16_set_byte(cpu->latch_addr, 0, bus);

    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr0(cpu, (uint8_t)(bus + cpu->X));

    return tick;
}

/* common_handler; role=w_indexed_probe; action=dummy_read_indexed_probe_then_fix_ea_high */
static qe6502_tick_t mc_w_indexed_probe(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t page_cross = cpu->latch_data;

    const uint16_t addr = u16_set_byte(cpu->latch_addr, 1, bus);
    qe6502_tick_t tick = read(cpu, addr);
    set_latch_addr1(cpu, (uint8_t)(bus + page_cross));

    return tick;
}

/* common_handler; role=read_zp_latch_x; action=dummy_read_zero_page_base_then_add_x_wraparound */
static qe6502_tick_t mc_read_zp_latch_x(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t addr = bus;

    qe6502_tick_t tick = read(cpu, addr);
    cpu->latch_addr = (uint8_t)(bus + cpu->X);

    return tick;
}

/* addressing_handler; role=ptrhi; action=read_zp_pointer_to_ea_high */
static qe6502_tick_t mc_w_izx_c3_ptrhi(qe6502_t* cpu, uint8_t bus)
{
    const uint16_t ptr_hi_addr = cpu->latch_addr;

    set_latch_addr0(cpu, bus);
    return read(cpu, ptr_hi_addr);
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
    const uint8_t ptr = u16_get_byte(cpu->latch_addr, 0);
    const uint16_t ptr_hi_addr = (uint8_t)(ptr + 1u);
    const uint16_t indexed = (uint16_t)(bus + cpu->Y);

    cpu->latch_addr = (uint16_t)(indexed & 0x00FFu);
    cpu->latch_data = (uint8_t)(indexed >> 8u);

    return read(cpu, ptr_hi_addr);
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
    adc_value(cpu, bus);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; model=nes; role=exec_fetch; action=execute_binary_adc_operand_and_fetch_next_opcode */
static qe6502_tick_t op_nes_adc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    adc_binary(cpu, bus);

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; model=wdc; role=exec_fetch_or_decimal_dummy; action=execute_binary_adc_or_latch_decimal_operand_and_read_effective_address_dummy */
static qe6502_tick_t op_wdc_adc_r_ready_addr_pending_data_fetch_or_decimal_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return mc_fetch(cpu, bus);
    }

    return read(cpu, cpu->latch_addr);
}

/* mnemonic_handler; model=wdc; role=exec_fetch_or_decimal_dummy; action=execute_binary_adc_or_latch_decimal_immediate_and_read_wdc_decimal_dummy */
static qe6502_tick_t op_wdc_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return mc_fetch(cpu, bus);
    }

    return read(cpu, 0x007fu);
}

/* mnemonic_handler; model=rw; role=exec_fetch_or_decimal_dummy; action=execute_binary_adc_or_latch_decimal_immediate_and_read_rockwell_decimal_dummy */
static qe6502_tick_t op_rw_adc_imm_ready_none_pending_data_fetch_or_decimal_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, bus);
        cpu->microcode++;
        return mc_fetch(cpu, bus);
    }

    return read(cpu, 0x0059u);
}

/* mnemonic_handler; model=wdc; role=exec_fetch; action=execute_cmos_decimal_adc_and_fetch_next_opcode */
static qe6502_tick_t op_wdc_adc_dec_ready_none_pending_dummy_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    adc_decimal_cmos(cpu, cpu->latch_data);
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

    cpu->A = asl_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_asl_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_data = asl_value(cpu, cpu->latch_data);
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

/* mnemonic_handler; model=wdc; role=exec_fetch; action=execute_immediate_bit_test_and_fetch_next_opcode */
static qe6502_tick_t op_wdc_bit_imm_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    const uint8_t flags = flag_Z_if((cpu->A & bus) == 0u);

    cpu->latch_data = bus;
    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
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

/* mnemonic_handler; model=wdc; role=offset; action=read_branch_offset_and_always_select_taken_path */
static qe6502_tick_t op_wdc_bra_branch_c0_offset(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return branch_c0_offset(cpu, true);
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

    cpu->latch_data--;
    update_flags_nz(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; model=wdc; role=exec_dummy; action=increment_accumulator_and_emit_dummy_pc_read */
static qe6502_tick_t op_wdc_inc_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A++;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; model=wdc; role=exec_dummy; action=decrement_accumulator_and_emit_dummy_pc_read */
static qe6502_tick_t op_wdc_dec_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A--;
    update_flags_nz(cpu, cpu->A);
    return read(cpu, cpu->PC);
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

    cpu->latch_data++;
    update_flags_nz(cpu, cpu->latch_data);
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

    cpu->A = lsr_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_lsr_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_data = lsr_value(cpu, cpu->latch_data);
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

/* mnemonic_handler; model=wdc; role=write; action=write_x_to_stack_and_decrement_s */
static qe6502_tick_t op_wdc_phx_stack_push_ready_none_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, cpu->X);
}

/* mnemonic_handler; model=wdc; role=write; action=write_y_to_stack_and_decrement_s */
static qe6502_tick_t op_wdc_phy_stack_push_ready_none_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    return stack_write(cpu, cpu->Y);
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

/* mnemonic_handler; model=wdc; role=exec_fetch; action=consume_pulled_stack_value_into_x_update_nz_and_fetch_next_opcode */
static qe6502_tick_t op_wdc_plx_stack_pull_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->X = bus;
    update_flags_nz(cpu, cpu->X);

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; model=wdc; role=exec_fetch; action=consume_pulled_stack_value_into_y_update_nz_and_fetch_next_opcode */
static qe6502_tick_t op_wdc_ply_stack_pull_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->Y = bus;
    update_flags_nz(cpu, cpu->Y);

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_rol_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A = rol_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_rol_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_data = rol_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_dummy; action=execute_accumulator_mnemonic_and_emit_dummy_pc_read */
static qe6502_tick_t op_ror_acc_ready_none_pending_none_dummy(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->A = ror_value(cpu, cpu->A);
    return read(cpu, cpu->PC);
}

/* mnemonic_handler; role=exec_write; action=execute_read_modify_write_mnemonic_and_emit_final_memory_write */
static qe6502_tick_t op_ror_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    cpu->latch_data = ror_value(cpu, cpu->latch_data);
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; role=exec_fetch; action=execute_read_operand_mnemonic_and_fetch_next_opcode */
static qe6502_tick_t op_sbc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;
    sbc_value(cpu, bus);
    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; model=nes; role=exec_fetch; action=execute_binary_sbc_operand_and_fetch_next_opcode */
static qe6502_tick_t op_nes_sbc_r_ready_none_pending_data_fetch(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    adc_binary(cpu, (uint8_t)(bus ^ 0xffu));

    return mc_fetch(cpu, bus);
}

/* mnemonic_handler; model=wdc; role=exec_fetch_or_decimal_dummy; action=execute_binary_sbc_or_latch_decimal_operand_and_read_effective_address_dummy */
static qe6502_tick_t op_wdc_sbc_r_ready_addr_pending_data_fetch_or_decimal_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(bus ^ 0xffu));
        cpu->microcode++;
        return mc_fetch(cpu, bus);
    }

    return read(cpu, cpu->latch_addr);
}

/* mnemonic_handler; model=wdc; role=exec_fetch_or_decimal_dummy; action=execute_binary_sbc_or_latch_decimal_immediate_and_read_wdc_decimal_dummy */
static qe6502_tick_t op_wdc_sbc_imm_ready_none_pending_data_fetch_or_decimal_dummy(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_data = bus;

    if ((cpu->P & flag_D) == 0u)
    {
        adc_binary(cpu, (uint8_t)(bus ^ 0xffu));
        cpu->microcode++;
        return mc_fetch(cpu, bus);
    }

    return read(cpu, 0x0000u);
}

/* mnemonic_handler; model=wdc; role=exec_fetch; action=execute_cmos_decimal_sbc_and_fetch_next_opcode */
static qe6502_tick_t op_wdc_sbc_dec_ready_none_pending_dummy_fetch(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    sbc_decimal_cmos(cpu, cpu->latch_data);
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

/* mnemonic_handler; model=wdc; role=exec_write; action=write_zero_to_effective_address */
static qe6502_tick_t op_wdc_stz_w_ready_addr_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;
    return write(cpu, cpu->latch_addr, (uint8_t)0u);
}

/* mnemonic_handler; model=wdc; role=exec_write; action=set_effective_address_high_and_write_zero */
static qe6502_tick_t op_wdc_stz_w_ready_addrlo_pending_addrhi_wr(qe6502_t* cpu, uint8_t bus)
{
    set_latch_addr1(cpu, bus);
    return write(cpu, cpu->latch_addr, (uint8_t)0u);
}

/* mnemonic_handler; model=wdc; role=exec_write; action=latch_zero_page_effective_address_and_write_zero */
static qe6502_tick_t op_wdc_stz_w_ready_none_pending_addrlo_wr(qe6502_t* cpu, uint8_t bus)
{
    cpu->latch_addr = bus;
    return write(cpu, cpu->latch_addr, (uint8_t)0u);
}

/* mnemonic_handler; model=wdc; role=exec_write; action=test_and_reset_bits_update_z_and_emit_final_memory_write */
static qe6502_tick_t op_wdc_trb_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint8_t flags = flag_Z_if((cpu->A & cpu->latch_data) == 0u);

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
    cpu->latch_data = (uint8_t)(cpu->latch_data & (uint8_t)(~cpu->A));
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; model=wdc; role=exec_write; action=test_and_set_bits_update_z_and_emit_final_memory_write */
static qe6502_tick_t op_wdc_tsb_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint8_t flags = flag_Z_if((cpu->A & cpu->latch_data) == 0u);

    cpu->P = (uint8_t)((cpu->P & (uint8_t)(~flag_Z)) | flags);
    cpu->latch_data = (uint8_t)(cpu->latch_data | cpu->A);
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}

/* mnemonic_handler; model=wdc; role=exec_write; action=reset_or_set_opcode_selected_zero_page_bit_and_emit_final_memory_write */
static qe6502_tick_t op_wdc_rmb_smb_rw_ready_addr_data_pending_none_wr(qe6502_t* cpu, uint8_t bus)
{
    (void)bus;

    const uint8_t opcode = current_opcode_slot(cpu);
    uint8_t bit = (uint8_t)(opcode >> 4u);

    if (bit < 8u)
    {
        const uint8_t mask = (uint8_t)(1u << bit);
        cpu->latch_data = (uint8_t)(cpu->latch_data & (uint8_t)(~mask));
    }
    else
    {
        bit = (uint8_t)(bit - 8u);
        const uint8_t mask = (uint8_t)(1u << bit);
        cpu->latch_data = (uint8_t)(cpu->latch_data | mask);
    }

    return write(cpu, cpu->latch_addr, cpu->latch_data);
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

const qe6502_microcode_fn qe6502_control_store[qe6502_control_store_size] =
{
#include "control_store/nmos_block.inc"
,
#include "control_store/nes_block.inc"
,
#include "control_store/wdc_block.inc"
,
#include "control_store/rw_block.inc"
};

#undef IDX
