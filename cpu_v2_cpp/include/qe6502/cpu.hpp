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

using tick = qe6502_tick_t;

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
    {
        cpu_.model = static_cast<std::uint8_t>(cpu_model);
    }

    tick light_reset()
    {
        return qe6502_v2_light_reset(&cpu_);
    }

    tick go_to(std::uint16_t address)
    {
        return qe6502_v2_goto(&cpu_, address);
    }

    tick step(std::uint8_t bus)
    {
        return qe6502_tick(&cpu_, bus);
    }

    model cpu_model() const
    {
        return static_cast<model>(cpu_.model);
    }

    void cpu_model(model value)
    {
        cpu_.model = static_cast<std::uint8_t>(value);
    }

    std::uint16_t pc() const { return cpu_.PC; }
    void pc(std::uint16_t value) { cpu_.PC = value; }

    std::uint8_t s() const { return cpu_.S; }
    void s(std::uint8_t value) { cpu_.S = value; }

    std::uint8_t a() const { return cpu_.A; }
    void a(std::uint8_t value) { cpu_.A = value; }

    std::uint8_t x() const { return cpu_.X; }
    void x(std::uint8_t value) { cpu_.X = value; }

    std::uint8_t y() const { return cpu_.Y; }
    void y(std::uint8_t value) { cpu_.Y = value; }

    std::uint8_t p() const { return cpu_.P; }
    void p(std::uint8_t value) { cpu_.P = value; }

    qe6502_t& raw()
    {
        return cpu_;
    }

    const qe6502_t& raw() const
    {
        return cpu_;
    }

private:
    qe6502_t cpu_{};
};

} // namespace qe6502

#endif // QE6502_CPP_CPU_HPP
