#include <asm6502/asm6502.h>

#include <cassert>
#include <cstdint>

int main()
{
    using namespace asm6502;

    Asm6502 p = Asm6502::New()
    .begin()
        .reset_vector("boot")
        .nmi_vector(0x1234)
        .brk_irq_vector("irq")
        .set("zp_ptr", 0x0080)
        .set("out", 0x0002)
        .set("table_plus_two", "table", +2)
        .org(0x0400, "boot")
            .lda(0x24)
            .ldx(0xfd)
            .txs()
            .pha()
            .lda(0x45)
            .ldx(0x05)
            .ldy(0x07)
            .plp()
            .jmp("test")
        .org(0x0080)
            .dw("table")
        .org(0x0500, "test")
            .label("loop")
            .lda(izy, "zp_ptr")
            .sta(zp, "out")
            .bne("loop")
            .jmp("done")
        .org(0x0600, "table")
            .db(0x34, 0x56, 0x78)
            .dw(0x12ab, "table_plus_two", sym("done", +1), 0x88cf)
        .org(0x0700, "done")
            .label("irq")
            .nop()
    .end();

    const auto out = p.compile_to_map();

    assert(out.at(0xfffc) == 0x00);
    assert(out.at(0xfffd) == 0x04);
    assert(out.at(0xfffa) == 0x34);
    assert(out.at(0xfffb) == 0x12);
    assert(out.at(0xfffe) == 0x00);
    assert(out.at(0xffff) == 0x07);

    assert(out.at(0x0400) == 0xa9);
    assert(out.at(0x0401) == 0x24);
    assert(out.at(0x040d) == 0x4c);
    assert(out.at(0x040e) == 0x00);
    assert(out.at(0x040f) == 0x05);

    assert(out.at(0x0080) == 0x00);
    assert(out.at(0x0081) == 0x06);

    assert(out.at(0x0500) == 0xb1);
    assert(out.at(0x0501) == 0x80);
    assert(out.at(0x0502) == 0x85);
    assert(out.at(0x0503) == 0x02);
    assert(out.at(0x0504) == 0xd0);
    assert(out.at(0x0505) == 0xfa); // 0x0500 - 0x0506 == -6

    assert(out.at(0x0603) == 0xab);
    assert(out.at(0x0604) == 0x12);
    assert(out.at(0x0605) == 0x02);
    assert(out.at(0x0606) == 0x06);
    assert(out.at(0x0607) == 0x01);
    assert(out.at(0x0608) == 0x07);
    assert(out.at(0x0609) == 0xcf);
    assert(out.at(0x060a) == 0x88);

    return 0;
}
