#include <asm6502/asm6502.h>

#include <iomanip>
#include <sstream>

namespace asm6502 {

symbol_ref sym(std::string name, int displacement)
{
    return symbol_ref{std::move(name), displacement};
}

Asm6502::Asm6502() = default;

std::string Asm6502::hex16(std::uint16_t value)
{
    std::ostringstream out;
    out << "$" << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
        << static_cast<unsigned>(value);
    return out.str();
}

std::string Asm6502::hex8(std::uint8_t value)
{
    std::ostringstream out;
    out << "$" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<unsigned>(value);
    return out.str();
}

void Asm6502::asm_context::put(std::uint16_t address, std::uint8_t value)
{
    if (output == nullptr)
        throw std::logic_error("asm_context has no output map");

    const auto [it, inserted] = output->emplace(address, value);
    if (!inserted)
    {
        throw std::runtime_error(
            "memory overwrite at " + hex16(address) +
            " old=" + hex8(it->second) +
            " new=" + hex8(value));
    }
}

bool Asm6502::define_symbol(symbol_table& symbols, const std::string& name, std::uint16_t value)
{
    const auto [it, inserted] = symbols.emplace(name, value);
    if (inserted)
        return true;

    if (it->second != value)
    {
        throw std::runtime_error(
            "symbol redefined with different value: " + name +
            " old=" + hex16(it->second) +
            " new=" + hex16(value));
    }

    return false;
}

std::uint16_t Asm6502::resolve_target(const asm_context& ctx, const link_target& target)
{
    return std::visit([&ctx](const auto& value) -> std::uint16_t {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;

        if constexpr (std::is_same_v<T, std::uint16_t>)
        {
            return value;
        }
        else if constexpr (std::is_same_v<T, symbol_ref>)
        {
            const auto it = ctx.symbols.find(value.name);
            if (it == ctx.symbols.end())
                throw std::runtime_error("undefined symbol: " + value.name);

            return static_cast<std::uint16_t>(static_cast<int>(it->second) + value.displacement);
        }
        else
        {
            static_assert(std::is_same_v<T, std::uint16_t>, "unsupported link target");
        }
    }, target);
}

Asm6502 Asm6502::New()
{
    return Asm6502{};
}

Asm6502 &Asm6502::begin()
{
    return *this;
}

Asm6502 &Asm6502::end()
{
    return *this;
}

Asm6502& Asm6502::org(std::uint16_t address)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [](asm_context&) {},
        [address](asm_context& ctx) { ctx.pc = address; }
    });
    return *this;
}

Asm6502& Asm6502::org(std::uint16_t address, std::string name)
{
    return org(address).label(std::move(name));
}

Asm6502& Asm6502::label(std::string name)
{
    commands_.push_back(asm_command{
        [name = std::move(name)](asm_context& ctx) {
            return define_symbol(ctx.symbols, name, ctx.pc);
        },
        [](asm_context&) {},
        [](asm_context&) {}
    });
    return *this;
}

Asm6502& Asm6502::set(std::string name, std::uint16_t value)
{
    commands_.push_back(asm_command{
        [name = std::move(name), value](asm_context& ctx) {
            return define_symbol(ctx.symbols, name, value);
        },
        [](asm_context&) {},
        [](asm_context&) {}
    });
    return *this;
}

Asm6502& Asm6502::set(std::string name, std::string base, int displacement)
{
    commands_.push_back(asm_command{
        [name = std::move(name), base = std::move(base), displacement](asm_context& ctx) {
            const auto it = ctx.symbols.find(base);
            if (it == ctx.symbols.end())
                return false;

            const auto value = static_cast<std::uint16_t>(static_cast<int>(it->second) + displacement);
            return define_symbol(ctx.symbols, name, value);
        },
        [](asm_context&) {},
        [](asm_context&) {}
    });
    return *this;
}

Asm6502& Asm6502::nmi_vector(std::uint16_t address)
{
    return add_fixed_word(0xfffau, link_target{address});
}

Asm6502& Asm6502::nmi_vector(std::string label, int displacement)
{
    return add_fixed_word(0xfffau, link_target{symbol_ref{std::move(label), displacement}});
}

Asm6502& Asm6502::reset_vector(std::uint16_t address)
{
    return add_fixed_word(0xfffcu, link_target{address});
}

Asm6502& Asm6502::reset_vector(std::string label, int displacement)
{
    return add_fixed_word(0xfffcu, link_target{symbol_ref{std::move(label), displacement}});
}

Asm6502& Asm6502::brk_irq_vector(std::uint16_t address)
{
    return add_fixed_word(0xfffeu, link_target{address});
}

Asm6502& Asm6502::brk_irq_vector(std::string label, int displacement)
{
    return add_fixed_word(0xfffeu, link_target{symbol_ref{std::move(label), displacement}});
}

Asm6502& Asm6502::add_byte(std::uint8_t value)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [value](asm_context& ctx) { ctx.put(ctx.pc, value); },
        [](asm_context& ctx) { ctx.pc = static_cast<std::uint16_t>(ctx.pc + 1u); }
    });
    return *this;
}

Asm6502& Asm6502::add_low(link_target target)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [target = std::move(target)](asm_context& ctx) {
            const std::uint16_t value = resolve_target(ctx, target);
            ctx.put(ctx.pc, static_cast<std::uint8_t>(value));
        },
        [](asm_context& ctx) { ctx.pc = static_cast<std::uint16_t>(ctx.pc + 1u); }
    });
    return *this;
}

Asm6502& Asm6502::add_word(link_target target)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [target = std::move(target)](asm_context& ctx) {
            const std::uint16_t value = resolve_target(ctx, target);
            ctx.put(ctx.pc, static_cast<std::uint8_t>(value));
            ctx.put(static_cast<std::uint16_t>(ctx.pc + 1u), static_cast<std::uint8_t>(value >> 8));
        },
        [](asm_context& ctx) { ctx.pc = static_cast<std::uint16_t>(ctx.pc + 2u); }
    });
    return *this;
}

Asm6502& Asm6502::add_relative(link_target target)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [target = std::move(target)](asm_context& ctx) {
            const std::uint16_t target_address = resolve_target(ctx, target);
            const std::uint16_t base = static_cast<std::uint16_t>(ctx.pc + 1u);
            int relative = static_cast<int>(target_address) - static_cast<int>(base);
            if (relative > 32767)
                relative -= 65536;
            else if (relative < -32768)
                relative += 65536;

            if (relative < -128 || relative > 127)
                throw std::out_of_range("relative branch target out of range");

            ctx.put(ctx.pc, static_cast<std::uint8_t>(relative));
        },
        [](asm_context& ctx) { ctx.pc = static_cast<std::uint16_t>(ctx.pc + 1u); }
    });
    return *this;
}

Asm6502& Asm6502::add_fixed_word(std::uint16_t address, link_target target)
{
    commands_.push_back(asm_command{
        [](asm_context&) { return false; },
        [address, target = std::move(target)](asm_context& ctx) {
            const std::uint16_t value = resolve_target(ctx, target);
            ctx.put(address, static_cast<std::uint8_t>(value));
            ctx.put(static_cast<std::uint16_t>(address + 1u), static_cast<std::uint8_t>(value >> 8));
        },
        [](asm_context&) {}
    });
    return *this;
}

Asm6502& Asm6502::emit_implied(std::uint8_t opcode)
{
    return add_byte(opcode);
}

Asm6502& Asm6502::emit_branch(std::uint8_t opcode, std::string label, int displacement)
{
    add_byte(opcode);
    add_relative(link_target{symbol_ref{std::move(label), displacement}});
    return *this;
}

Asm6502& Asm6502::emit_addressed(const char* mnemonic, address_mode mode, link_target target,
                                 const mode_opcode* table, std::size_t table_size)
{
    for (std::size_t i = 0; i < table_size; ++i)
    {
        if (table[i].mode == mode.id)
        {
            add_byte(table[i].opcode);
            if (table[i].word_operand)
                return add_word(std::move(target));
            return add_low(std::move(target));
        }
    }

    throw std::invalid_argument(std::string("unsupported ") + mnemonic + " addressing mode");
}

#define TABLE_SIZE(table) (sizeof(table) / sizeof((table)[0]))

Asm6502& Asm6502::adc_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0x69u, false}, {address_mode_id::zp, 0x65u, false},
        {address_mode_id::zpx, 0x75u, false}, {address_mode_id::abs, 0x6du, true},
        {address_mode_id::abx, 0x7du, true}, {address_mode_id::aby, 0x79u, true},
        {address_mode_id::izx, 0x61u, false}, {address_mode_id::izy, 0x71u, false}
    };
    return emit_addressed("ADC", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::and_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0x29u, false}, {address_mode_id::zp, 0x25u, false},
        {address_mode_id::zpx, 0x35u, false}, {address_mode_id::abs, 0x2du, true},
        {address_mode_id::abx, 0x3du, true}, {address_mode_id::aby, 0x39u, true},
        {address_mode_id::izx, 0x21u, false}, {address_mode_id::izy, 0x31u, false}
    };
    return emit_addressed("AND", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::bit_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::zp, 0x24u, false}, {address_mode_id::abs, 0x2cu, true}
    };
    return emit_addressed("BIT", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::cmp_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xc9u, false}, {address_mode_id::zp, 0xc5u, false},
        {address_mode_id::zpx, 0xd5u, false}, {address_mode_id::abs, 0xcdu, true},
        {address_mode_id::abx, 0xddu, true}, {address_mode_id::aby, 0xd9u, true},
        {address_mode_id::izx, 0xc1u, false}, {address_mode_id::izy, 0xd1u, false}
    };
    return emit_addressed("CMP", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::cpx_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xe0u, false}, {address_mode_id::zp, 0xe4u, false},
        {address_mode_id::abs, 0xecu, true}
    };
    return emit_addressed("CPX", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::cpy_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xc0u, false}, {address_mode_id::zp, 0xc4u, false},
        {address_mode_id::abs, 0xccu, true}
    };
    return emit_addressed("CPY", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::dec_impl(address_mode mode, link_target target)
{
    return rmw_impl("DEC", mode, std::move(target), 0xc6u, 0xd6u, 0xceu, 0xdeu);
}

Asm6502& Asm6502::eor_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0x49u, false}, {address_mode_id::zp, 0x45u, false},
        {address_mode_id::zpx, 0x55u, false}, {address_mode_id::abs, 0x4du, true},
        {address_mode_id::abx, 0x5du, true}, {address_mode_id::aby, 0x59u, true},
        {address_mode_id::izx, 0x41u, false}, {address_mode_id::izy, 0x51u, false}
    };
    return emit_addressed("EOR", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::inc_impl(address_mode mode, link_target target)
{
    return rmw_impl("INC", mode, std::move(target), 0xe6u, 0xf6u, 0xeeu, 0xfeu);
}

Asm6502& Asm6502::jmp_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::abs, 0x4cu, true}, {address_mode_id::ind, 0x6cu, true}
    };
    return emit_addressed("JMP", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::lda_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xa9u, false}, {address_mode_id::zp, 0xa5u, false},
        {address_mode_id::zpx, 0xb5u, false}, {address_mode_id::abs, 0xadu, true},
        {address_mode_id::abx, 0xbdu, true}, {address_mode_id::aby, 0xb9u, true},
        {address_mode_id::izx, 0xa1u, false}, {address_mode_id::izy, 0xb1u, false}
    };
    return emit_addressed("LDA", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::ldx_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xa2u, false}, {address_mode_id::zp, 0xa6u, false},
        {address_mode_id::zpy, 0xb6u, false}, {address_mode_id::abs, 0xaeu, true},
        {address_mode_id::aby, 0xbeu, true}
    };
    return emit_addressed("LDX", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::ldy_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xa0u, false}, {address_mode_id::zp, 0xa4u, false},
        {address_mode_id::zpx, 0xb4u, false}, {address_mode_id::abs, 0xacu, true},
        {address_mode_id::abx, 0xbcu, true}
    };
    return emit_addressed("LDY", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::ora_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0x09u, false}, {address_mode_id::zp, 0x05u, false},
        {address_mode_id::zpx, 0x15u, false}, {address_mode_id::abs, 0x0du, true},
        {address_mode_id::abx, 0x1du, true}, {address_mode_id::aby, 0x19u, true},
        {address_mode_id::izx, 0x01u, false}, {address_mode_id::izy, 0x11u, false}
    };
    return emit_addressed("ORA", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::rmw_impl(const char* mnemonic, address_mode mode, link_target target,
                           std::uint8_t zp_opcode, std::uint8_t zpx_opcode,
                           std::uint8_t abs_opcode, std::uint8_t abx_opcode)
{
    const mode_opcode table[] = {
        {address_mode_id::zp, zp_opcode, false}, {address_mode_id::zpx, zpx_opcode, false},
        {address_mode_id::abs, abs_opcode, true}, {address_mode_id::abx, abx_opcode, true}
    };
    return emit_addressed(mnemonic, mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::sbc_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::imm, 0xe9u, false}, {address_mode_id::zp, 0xe5u, false},
        {address_mode_id::zpx, 0xf5u, false}, {address_mode_id::abs, 0xedu, true},
        {address_mode_id::abx, 0xfdu, true}, {address_mode_id::aby, 0xf9u, true},
        {address_mode_id::izx, 0xe1u, false}, {address_mode_id::izy, 0xf1u, false}
    };
    return emit_addressed("SBC", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::sta_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::zp, 0x85u, false}, {address_mode_id::zpx, 0x95u, false},
        {address_mode_id::abs, 0x8du, true}, {address_mode_id::abx, 0x9du, true},
        {address_mode_id::aby, 0x99u, true}, {address_mode_id::izx, 0x81u, false},
        {address_mode_id::izy, 0x91u, false}
    };
    return emit_addressed("STA", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::stx_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::zp, 0x86u, false}, {address_mode_id::zpy, 0x96u, false},
        {address_mode_id::abs, 0x8eu, true}
    };
    return emit_addressed("STX", mode, std::move(target), table, TABLE_SIZE(table));
}

Asm6502& Asm6502::sty_impl(address_mode mode, link_target target)
{
    static constexpr mode_opcode table[] = {
        {address_mode_id::zp, 0x84u, false}, {address_mode_id::zpx, 0x94u, false},
        {address_mode_id::abs, 0x8cu, true}
    };
    return emit_addressed("STY", mode, std::move(target), table, TABLE_SIZE(table));
}

#undef TABLE_SIZE

// Public addressed wrappers.
Asm6502& Asm6502::adc(address_mode mode, std::uint16_t operand) { return adc_impl(mode, link_target{operand}); }
Asm6502& Asm6502::adc(address_mode mode, std::string label, int displacement) { return adc_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::and_(address_mode mode, std::uint16_t operand) { return and_impl(mode, link_target{operand}); }
Asm6502& Asm6502::and_(address_mode mode, std::string label, int displacement) { return and_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::bit(address_mode mode, std::uint16_t operand) { return bit_impl(mode, link_target{operand}); }
Asm6502& Asm6502::bit(address_mode mode, std::string label, int displacement) { return bit_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::cmp(address_mode mode, std::uint16_t operand) { return cmp_impl(mode, link_target{operand}); }
Asm6502& Asm6502::cmp(address_mode mode, std::string label, int displacement) { return cmp_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::cpx(address_mode mode, std::uint16_t operand) { return cpx_impl(mode, link_target{operand}); }
Asm6502& Asm6502::cpx(address_mode mode, std::string label, int displacement) { return cpx_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::cpy(address_mode mode, std::uint16_t operand) { return cpy_impl(mode, link_target{operand}); }
Asm6502& Asm6502::cpy(address_mode mode, std::string label, int displacement) { return cpy_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::dec(address_mode mode, std::uint16_t operand) { return dec_impl(mode, link_target{operand}); }
Asm6502& Asm6502::dec(address_mode mode, std::string label, int displacement) { return dec_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::eor(address_mode mode, std::uint16_t operand) { return eor_impl(mode, link_target{operand}); }
Asm6502& Asm6502::eor(address_mode mode, std::string label, int displacement) { return eor_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::inc(address_mode mode, std::uint16_t operand) { return inc_impl(mode, link_target{operand}); }
Asm6502& Asm6502::inc(address_mode mode, std::string label, int displacement) { return inc_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::jmp(std::uint16_t address) { return jmp_impl(absolute, link_target{address}); }
Asm6502& Asm6502::jmp(std::string label, int displacement) { return jmp_impl(absolute, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::jmp(address_mode mode, std::uint16_t address) { return jmp_impl(mode, link_target{address}); }
Asm6502& Asm6502::jmp(address_mode mode, std::string label, int displacement) { return jmp_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::jsr(std::uint16_t address) { add_byte(0x20u); return add_word(link_target{address}); }
Asm6502& Asm6502::jsr(std::string label, int displacement) { add_byte(0x20u); return add_word(link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::lda(address_mode mode, std::uint16_t operand) { return lda_impl(mode, link_target{operand}); }
Asm6502& Asm6502::lda(address_mode mode, std::string label, int displacement) { return lda_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::ldx(address_mode mode, std::uint16_t operand) { return ldx_impl(mode, link_target{operand}); }
Asm6502& Asm6502::ldx(address_mode mode, std::string label, int displacement) { return ldx_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::ldy(address_mode mode, std::uint16_t operand) { return ldy_impl(mode, link_target{operand}); }
Asm6502& Asm6502::ldy(address_mode mode, std::string label, int displacement) { return ldy_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::ora(address_mode mode, std::uint16_t operand) { return ora_impl(mode, link_target{operand}); }
Asm6502& Asm6502::ora(address_mode mode, std::string label, int displacement) { return ora_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::sbc(address_mode mode, std::uint16_t operand) { return sbc_impl(mode, link_target{operand}); }
Asm6502& Asm6502::sbc(address_mode mode, std::string label, int displacement) { return sbc_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::sta(address_mode mode, std::uint16_t operand) { return sta_impl(mode, link_target{operand}); }
Asm6502& Asm6502::sta(address_mode mode, std::string label, int displacement) { return sta_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::stx(address_mode mode, std::uint16_t operand) { return stx_impl(mode, link_target{operand}); }
Asm6502& Asm6502::stx(address_mode mode, std::string label, int displacement) { return stx_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }
Asm6502& Asm6502::sty(address_mode mode, std::uint16_t operand) { return sty_impl(mode, link_target{operand}); }
Asm6502& Asm6502::sty(address_mode mode, std::string label, int displacement) { return sty_impl(mode, link_target{symbol_ref{std::move(label), displacement}}); }

// Accumulator/read-modify-write instructions.
Asm6502& Asm6502::asl() { return emit_implied(0x0au); }
Asm6502& Asm6502::asl(address_mode mode, std::uint16_t operand) { return rmw_impl("ASL", mode, link_target{operand}, 0x06u, 0x16u, 0x0eu, 0x1eu); }
Asm6502& Asm6502::asl(address_mode mode, std::string label, int displacement) { return rmw_impl("ASL", mode, link_target{symbol_ref{std::move(label), displacement}}, 0x06u, 0x16u, 0x0eu, 0x1eu); }
Asm6502& Asm6502::lsr() { return emit_implied(0x4au); }
Asm6502& Asm6502::lsr(address_mode mode, std::uint16_t operand) { return rmw_impl("LSR", mode, link_target{operand}, 0x46u, 0x56u, 0x4eu, 0x5eu); }
Asm6502& Asm6502::lsr(address_mode mode, std::string label, int displacement) { return rmw_impl("LSR", mode, link_target{symbol_ref{std::move(label), displacement}}, 0x46u, 0x56u, 0x4eu, 0x5eu); }
Asm6502& Asm6502::rol() { return emit_implied(0x2au); }
Asm6502& Asm6502::rol(address_mode mode, std::uint16_t operand) { return rmw_impl("ROL", mode, link_target{operand}, 0x26u, 0x36u, 0x2eu, 0x3eu); }
Asm6502& Asm6502::rol(address_mode mode, std::string label, int displacement) { return rmw_impl("ROL", mode, link_target{symbol_ref{std::move(label), displacement}}, 0x26u, 0x36u, 0x2eu, 0x3eu); }
Asm6502& Asm6502::ror() { return emit_implied(0x6au); }
Asm6502& Asm6502::ror(address_mode mode, std::uint16_t operand) { return rmw_impl("ROR", mode, link_target{operand}, 0x66u, 0x76u, 0x6eu, 0x7eu); }
Asm6502& Asm6502::ror(address_mode mode, std::string label, int displacement) { return rmw_impl("ROR", mode, link_target{symbol_ref{std::move(label), displacement}}, 0x66u, 0x76u, 0x6eu, 0x7eu); }

// Branches.
Asm6502& Asm6502::bcc(std::string label, int displacement) { return emit_branch(0x90u, std::move(label), displacement); }
Asm6502& Asm6502::bcs(std::string label, int displacement) { return emit_branch(0xb0u, std::move(label), displacement); }
Asm6502& Asm6502::beq(std::string label, int displacement) { return emit_branch(0xf0u, std::move(label), displacement); }
Asm6502& Asm6502::bmi(std::string label, int displacement) { return emit_branch(0x30u, std::move(label), displacement); }
Asm6502& Asm6502::bne(std::string label, int displacement) { return emit_branch(0xd0u, std::move(label), displacement); }
Asm6502& Asm6502::bpl(std::string label, int displacement) { return emit_branch(0x10u, std::move(label), displacement); }
Asm6502& Asm6502::bvc(std::string label, int displacement) { return emit_branch(0x50u, std::move(label), displacement); }
Asm6502& Asm6502::bvs(std::string label, int displacement) { return emit_branch(0x70u, std::move(label), displacement); }

// Implied instructions.
Asm6502& Asm6502::brk() { return emit_implied(0x00u); }
Asm6502& Asm6502::clc() { return emit_implied(0x18u); }
Asm6502& Asm6502::cld() { return emit_implied(0xd8u); }
Asm6502& Asm6502::cli() { return emit_implied(0x58u); }
Asm6502& Asm6502::clv() { return emit_implied(0xb8u); }
Asm6502& Asm6502::dex() { return emit_implied(0xcau); }
Asm6502& Asm6502::dey() { return emit_implied(0x88u); }
Asm6502& Asm6502::inx() { return emit_implied(0xe8u); }
Asm6502& Asm6502::iny() { return emit_implied(0xc8u); }
Asm6502& Asm6502::nop() { return emit_implied(0xeau); }
Asm6502& Asm6502::pha() { return emit_implied(0x48u); }
Asm6502& Asm6502::php() { return emit_implied(0x08u); }
Asm6502& Asm6502::pla() { return emit_implied(0x68u); }
Asm6502& Asm6502::plp() { return emit_implied(0x28u); }
Asm6502& Asm6502::rti() { return emit_implied(0x40u); }
Asm6502& Asm6502::rts() { return emit_implied(0x60u); }
Asm6502& Asm6502::sec() { return emit_implied(0x38u); }
Asm6502& Asm6502::sed() { return emit_implied(0xf8u); }
Asm6502& Asm6502::sei() { return emit_implied(0x78u); }
Asm6502& Asm6502::tax() { return emit_implied(0xaau); }
Asm6502& Asm6502::tay() { return emit_implied(0xa8u); }
Asm6502& Asm6502::tsx() { return emit_implied(0xbau); }
Asm6502& Asm6502::txa() { return emit_implied(0x8au); }
Asm6502& Asm6502::txs() { return emit_implied(0x9au); }
Asm6502& Asm6502::tya() { return emit_implied(0x98u); }

memory_modifiers Asm6502::compile_to_map() const
{
    asm_context resolved;

    for (;;)
    {
        symbol_table symbols = std::move(resolved.symbols);
        resolved = asm_context{};
        resolved.symbols = std::move(symbols);

        bool changed = false;
        for (const asm_command& cmd : commands_)
        {
            changed = cmd.resolve(resolved) || changed;
            cmd.update_pc(resolved);
        }

        if (!changed)
            break;
    }

    memory_modifiers output;
    asm_context placed;
    placed.symbols = std::move(resolved.symbols);
    placed.output = &output;

    for (const asm_command& cmd : commands_)
    {
        cmd.place(placed);
        cmd.update_pc(placed);
    }

    return output;
}

std::vector<mem_value> Asm6502::compile() const
{
    const memory_modifiers map = compile_to_map();
    return std::vector<mem_value>(map.begin(), map.end());
}

void Asm6502::apply(const std::vector<mem_value>& changes, std::uint8_t* memory)
{
    if (memory == nullptr)
        throw std::invalid_argument("compile() memory pointer is null");

    for (const auto& [address, value] : changes)
        memory[address] = value;
}

void Asm6502::apply(const memory_modifiers& map, std::uint8_t* memory)
{
    if (memory == nullptr)
        throw std::invalid_argument("compile() memory pointer is null");

    for (const auto& [address, value] : map)
        memory[address] = value;
}

std::vector<mem_value> bootstrap_program(std::uint8_t A,
                                         std::uint8_t X,
                                         std::uint8_t Y,
                                         std::uint8_t P,
                                         std::uint8_t S,
                                         std::uint16_t start_at,
                                         std::uint16_t reset_vector)
{
    Asm6502 program = Asm6502::New()
    .begin()
        .reset_vector(reset_vector)
        .org(reset_vector, "bootstrap")
            .ldx(S)
            .txs()
            .lda(P)
            .pha()
            .lda(A)
            .ldx(X)
            .ldy(Y)
            .plp()
            .jmp(start_at)
    .end();

    return program.compile();
}

} // namespace asm6502
