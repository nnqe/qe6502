#include <qe6502.h>
#include <stdio.h>

const char* test_klaus2m5(uint8_t cpu_model,
                          uint8_t* memory,
                          uint16_t success_address,
                          uint64_t expected_cycles,
                          uint8_t* result);

void copy_klaus2m5_image( uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles );
void copy_klaus2m5_extended_image( uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles );

int main(void)
{
    uint16_t success_address = 0;
    uint64_t expected_cycles = 0;
    uint8_t memory[0x10000];
    uint8_t result = 0;
    const char* msg;
#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    copy_klaus2m5_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_MOS, memory, success_address, expected_cycles, &result);
    printf("MOS 6502 CPU normal test            %s : %s\n", result?"[PASS]":"[FAIL]", msg);
#endif
    //
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    copy_klaus2m5_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_WDC, memory, success_address, expected_cycles, &result);
    printf("WDC 65C02 CPU normal test           %s : %s\n", result?"[PASS]":"[FAIL]", msg);
    //
    copy_klaus2m5_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_RW, memory, success_address, expected_cycles, &result);
    printf("Rockwell 65C02 CPU normal test      %s : %s\n", result?"[PASS]":"[FAIL]", msg);
    //
    copy_klaus2m5_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_ST, memory, success_address, expected_cycles, &result);
    printf("Synertek 65C02 CPU normal test      %s : %s\n", result?"[PASS]":"[FAIL]", msg);

    // Extended
    copy_klaus2m5_extended_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_WDC, memory, success_address, expected_cycles, &result);
    printf("WDC 65C02 CPU extended test         %s : %s\n", result?"[PASS]":"[FAIL]", msg);
    //
    copy_klaus2m5_extended_image(memory, &success_address, &expected_cycles);
    msg = test_klaus2m5(QE6502_MODEL_RW, memory, success_address, expected_cycles, &result);
    printf("Rockwell 65C02 CPU extended test    %s : %s\n", result?"[PASS]":"[FAIL]", msg);
#endif
    return 0;
}
