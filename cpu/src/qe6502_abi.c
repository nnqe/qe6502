#include <qe6502/qe6502_abi.h>
#include <qe6502/qe6502.h>

typedef struct qe6502abi_impl
{
    qe6502_t cpu;
    uint16_t magic;
    uint16_t version;
    uint8_t reserve[44];
} qe6502abi_impl_t;

QE6502_STATIC_ASSERT(sizeof(qe6502_t) == 16u,
                     "qe6502 CPU context must be 16 bytes");
QE6502_STATIC_ASSERT(sizeof(qe6502abi_impl_t) == QE6502_ABI_CONTEXT_SIZE,
                     "qe6502 ABI implementation context must be 64 bytes");

static inline qe6502abi_impl_t *qe6502abi_impl(qe6502abi_context_t *ctx)
{
    return (qe6502abi_impl_t *)(void *)ctx;
}

static inline const qe6502abi_impl_t *qe6502abi_const_impl(const qe6502abi_context_t *ctx)
{
    return (const qe6502abi_impl_t *)(const void *)ctx;
}

static void qe6502abi_clear_bytes(uint8_t *bytes, uint32_t count)
{
    uint32_t i;
    for(i = 0u; i < count; ++i) {
        bytes[i] = 0u;
    }
}

static void qe6502abi_write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(((uint16_t)value >> 8u) & 0xffu);
    dst[1] = (uint8_t)((uint16_t)value & 0xffu);
}

static uint16_t qe6502abi_read_u16(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8u) | (uint16_t)src[1]);
}

static void qe6502abi_write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)((value >> 24u) & 0xffu);
    dst[1] = (uint8_t)((value >> 16u) & 0xffu);
    dst[2] = (uint8_t)((value >> 8u) & 0xffu);
    dst[3] = (uint8_t)(value & 0xffu);
}

static uint32_t qe6502abi_read_u32(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24u) |
           ((uint32_t)src[1] << 16u) |
           ((uint32_t)src[2] << 8u) |
           ((uint32_t)src[3]);
}

static void qe6502abi_clear_cpu(qe6502_t *cpu)
{
    cpu->model = 0u;
    cpu->status = 0u;
    cpu->microcode = 0u;
    cpu->latch_addr = 0u;
    cpu->latch_data = 0u;
    cpu->reserved_for_extension = 0u;
    cpu->PC = 0u;
    cpu->S = 0u;
    cpu->A = 0u;
    cpu->X = 0u;
    cpu->Y = 0u;
    cpu->P = 0u;
    cpu->interrupts = 0u;
}

static void qe6502abi_write_cpu(uint8_t *dst, const qe6502_t *cpu)
{
    dst[0] = cpu->model;
    dst[1] = cpu->status;
    qe6502abi_write_u16(&dst[2], cpu->microcode);
    qe6502abi_write_u16(&dst[4], cpu->latch_addr);
    dst[6] = cpu->latch_data;
    dst[7] = cpu->reserved_for_extension;
    qe6502abi_write_u16(&dst[8], cpu->PC);
    dst[10] = cpu->S;
    dst[11] = cpu->A;
    dst[12] = cpu->X;
    dst[13] = cpu->Y;
    dst[14] = cpu->P;
    dst[15] = cpu->interrupts;
}

static void qe6502abi_read_cpu(qe6502_t *cpu, const uint8_t *src)
{
    cpu->model = src[0];
    cpu->status = src[1];
    cpu->microcode = qe6502abi_read_u16(&src[2]);
    cpu->latch_addr = qe6502abi_read_u16(&src[4]);
    cpu->latch_data = src[6];
    cpu->reserved_for_extension = src[7];
    cpu->PC = qe6502abi_read_u16(&src[8]);
    cpu->S = src[10];
    cpu->A = src[11];
    cpu->X = src[12];
    cpu->Y = src[13];
    cpu->P = src[14];
    cpu->interrupts = src[15];
}

static inline qe6502abi_tick_t qe6502abi_pack_tick(qe6502_tick_t tick)
{
    return ((uint32_t)tick.address << QE6502_ABI_TICK_ADDRESS_SHIFT) |
           ((uint32_t)tick.bus << QE6502_ABI_TICK_BUS_SHIFT) |
           ((uint32_t)tick.status << QE6502_ABI_TICK_STATUS_SHIFT);
}

QE6502_ABI_API uint32_t qe6502abi_version(void)
{
    return QE6502_ABI_VERSION;
}

QE6502_ABI_API void qe6502abi_setup(qe6502abi_context_t *ctx, uint32_t model)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502abi_clear_cpu(&impl->cpu);
    impl->magic = (uint16_t)QE6502_ABI_CONTEXT_MAGIC;
    impl->version = (uint16_t)QE6502_ABI_CONTEXT_VERSION;
    qe6502abi_clear_bytes(impl->reserve, (uint32_t)sizeof(impl->reserve));
    impl->cpu.model = (uint8_t)model;
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_restart(qe6502abi_context_t *ctx)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return qe6502abi_pack_tick(qe6502_restart(&impl->cpu));
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_tick(qe6502abi_context_t *ctx, uint32_t bus)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return qe6502abi_pack_tick(qe6502_tick(&impl->cpu, (uint8_t)bus));
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_reset(qe6502abi_context_t *ctx)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return qe6502abi_pack_tick(qe6502_reset(&impl->cpu));
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_goto(qe6502abi_context_t *ctx, uint32_t address)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    return qe6502abi_pack_tick(qe6502_goto(&impl->cpu, (uint16_t)address));
}

QE6502_ABI_API void qe6502abi_nmi(qe6502abi_context_t *ctx)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502_nmi(&impl->cpu);
}

QE6502_ABI_API void qe6502abi_set_irq(qe6502abi_context_t *ctx, uint32_t pin)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    qe6502_set_irq(&impl->cpu, (uint8_t)pin);
}

QE6502_ABI_API uint32_t qe6502abi_get_irq(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return qe6502_get_irq(&impl->cpu);
}

QE6502_ABI_API void qe6502abi_save(const qe6502abi_context_t *ctx,
                                   qe6502abi_tick_t tick,
                                   uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE])
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);

    qe6502abi_write_u16(&snapshot[0], impl->magic);
    qe6502abi_write_u16(&snapshot[2], impl->version);
    qe6502abi_write_cpu(&snapshot[4], &impl->cpu);
    qe6502abi_write_u32(&snapshot[20], tick);
    qe6502abi_clear_bytes(&snapshot[24], QE6502_ABI_SNAPSHOT_SIZE - 24u);
}

QE6502_ABI_API qe6502abi_tick_t qe6502abi_load(qe6502abi_context_t *ctx,
                                               const uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE])
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);

    impl->magic = qe6502abi_read_u16(&snapshot[0]);
    impl->version = qe6502abi_read_u16(&snapshot[2]);
    qe6502abi_read_cpu(&impl->cpu, &snapshot[4]);
    const qe6502abi_tick_t tick = qe6502abi_read_u32(&snapshot[20]);
    qe6502abi_clear_bytes(impl->reserve, (uint32_t)sizeof(impl->reserve));

    return tick;
}

QE6502_ABI_API uint32_t qe6502abi_get_pc(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.PC;
}

QE6502_ABI_API void qe6502abi_set_pc(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.PC = (uint16_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_s(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.S;
}

QE6502_ABI_API void qe6502abi_set_s(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.S = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_a(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.A;
}

QE6502_ABI_API void qe6502abi_set_a(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.A = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_x(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.X;
}

QE6502_ABI_API void qe6502abi_set_x(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.X = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_y(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.Y;
}

QE6502_ABI_API void qe6502abi_set_y(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.Y = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_p(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.P;
}

QE6502_ABI_API void qe6502abi_set_p(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.P = (uint8_t)value;
}

QE6502_ABI_API uint32_t qe6502abi_get_model(const qe6502abi_context_t *ctx)
{
    const qe6502abi_impl_t *impl = qe6502abi_const_impl(ctx);
    return impl->cpu.model;
}

QE6502_ABI_API void qe6502abi_set_model(qe6502abi_context_t *ctx, uint32_t value)
{
    qe6502abi_impl_t *impl = qe6502abi_impl(ctx);
    impl->cpu.model = (uint8_t)value;
}
