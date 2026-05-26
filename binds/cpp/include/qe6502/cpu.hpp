#ifndef QE6502_CPP_CPU_HPP
#define QE6502_CPP_CPU_HPP

#include "qe6502.h"

#include <cstdint>

namespace qe6502 {

enum class model : std::uint8_t {
    nmos = qe6502_model_nmos,
    nes  = qe6502_model_nes,
    wdc  = qe6502_model_wdc,
    rw   = qe6502_model_rw,
    st   = qe6502_model_st,
};

inline constexpr std::uint8_t flag_c  = qe6502_flag_C;
inline constexpr std::uint8_t flag_z  = qe6502_flag_Z;
inline constexpr std::uint8_t flag_i  = qe6502_flag_I;
inline constexpr std::uint8_t flag_d  = qe6502_flag_D;
inline constexpr std::uint8_t flag_b  = qe6502_flag_B;
inline constexpr std::uint8_t flag_un = qe6502_flag_UN;
inline constexpr std::uint8_t flag_v  = qe6502_flag_V;
inline constexpr std::uint8_t flag_n  = qe6502_flag_N;

class cpu {
public:
    constexpr cpu() = default;

    explicit constexpr cpu(model cpu_model)
        : cpu_{}
        , tick_{}
    {
        cpu_.model = static_cast<std::uint8_t>(cpu_model);
    }

    void reset() noexcept
    {
        tick_ = qe6502_reset(&cpu_);
    }

    void go_to(std::uint16_t address) noexcept
    {
        tick_ = qe6502_goto(&cpu_, address);
    }

    void step(std::uint8_t bus) noexcept
    {
        tick_ = qe6502_tick(&cpu_, bus);
    }

    std::uint16_t address() const noexcept { return tick_.address; }
    std::uint8_t data() const noexcept { return tick_.bus; }
    std::uint8_t status() const noexcept { return tick_.status; }

    bool reading() const noexcept
    {
        return !writing();
    }

    bool writing() const noexcept
    {
        return (tick_.status & qe6502_status_writing) != 0u;
    }

    bool fetching() const noexcept
    {
        return (tick_.status & qe6502_status_opcode_fetch) != 0u;
    }

    bool trapped() const noexcept
    {
        return (tick_.status & qe6502_status_trapped) != 0u;
    }

    model active_model() const noexcept
    {
        return static_cast<model>(cpu_.model);
    }

    void active_model(model value) noexcept
    {
        cpu_.model = static_cast<std::uint8_t>(value);
    }

    std::uint16_t pc() const noexcept { return cpu_.PC; }
    void pc(std::uint16_t value) noexcept { cpu_.PC = value; }

    std::uint8_t s() const noexcept { return cpu_.S; }
    void s(std::uint8_t value) noexcept { cpu_.S = value; }

    std::uint8_t a() const noexcept { return cpu_.A; }
    void a(std::uint8_t value) noexcept { cpu_.A = value; }

    std::uint8_t x() const noexcept { return cpu_.X; }
    void x(std::uint8_t value) noexcept { cpu_.X = value; }

    std::uint8_t y() const noexcept { return cpu_.Y; }
    void y(std::uint8_t value) noexcept { cpu_.Y = value; }

    std::uint8_t p() const noexcept { return cpu_.P; }
    void p(std::uint8_t value) noexcept { cpu_.P = value; }

    qe6502_t& raw_cpu() noexcept
    {
        return cpu_;
    }

    const qe6502_t& raw_cpu() const noexcept
    {
        return cpu_;
    }

    qe6502_tick_t& raw_tick() noexcept
    {
        return tick_;
    }

    const qe6502_tick_t& raw_tick() const noexcept
    {
        return tick_;
    }

private:
    qe6502_t cpu_{};
    qe6502_tick_t tick_{};
};

} // namespace qe6502

#endif // QE6502_CPP_CPU_HPP
