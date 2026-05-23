#include <stdint.h>
#include <string.h>

static const uint8_t s_nmos_klaus2m5_rom[0x10000] =
    #include "../klaus2m5/6502_functional_test.hex"

void copy_klaus2m5_image(uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles)
{
    memcpy(dst, s_nmos_klaus2m5_rom, sizeof(s_nmos_klaus2m5_rom));
    *success_address = 0x3469;
    *expected_cycles = 30646176;
}
