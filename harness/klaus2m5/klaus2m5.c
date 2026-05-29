#include "qe6502.h"
#include <stdint.h>
#include <string.h>

static uint8_t tick_is_write(qe6502_tick_t tick)
{
    return (uint8_t)((tick.status & qe6502_status_writing) != 0u);
}

static uint8_t tick_is_opcode_fetch(qe6502_tick_t tick)
{
    return (uint8_t)((tick.status & qe6502_status_opcode_fetch) != 0u);
}

static uint8_t tick_bus_data(qe6502_tick_t tick, uint8_t* memory)
{
    if (tick_is_write(tick))
    {
        return tick.bus;
    }

    return memory[tick.address];
}

const char* test_klaus2m5_v2(uint8_t cpu_model,
                             uint8_t* memory,
                             uint16_t success_address,
                             uint64_t expected_cycles,
                             uint8_t* result)
{
    *result = 0;

    qe6502_t cpu;
    qe6502_t* cpu_ptr = &cpu;
    memset(cpu_ptr, 0, sizeof(cpu));
    cpu.model = cpu_model;

    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;

    qe6502_tick_t tick = qe6502_restart(cpu_ptr);

    while (!tick_is_opcode_fetch(tick))
    {
        const uint8_t data = tick_bus_data(tick, memory);
        if (tick_is_write(tick))
        {
            memory[tick.address] = data;
        }
        tick = qe6502_tick(cpu_ptr, data);
    }


    uint64_t cycles = 0;
    const char* result_msg = "CPU Error";

    for (;;)
    {
        uint8_t data = tick_bus_data(tick, memory);

        if (tick.address == success_address)
        {
            *result = 1;
            result_msg = "OK";
            break;
        }

        if (tick_is_write(tick))
        {
            memory[tick.address] = data;
        }
        else
        {
            data = memory[tick.address];
        }

        tick = qe6502_tick(cpu_ptr, data);

        if (tick_is_opcode_fetch(tick))
        {
            cycles++;
            if (cycles > 2u * expected_cycles)
            {
                result_msg = "Test fail, takes too many cycles!";
                break;
            }
        }
    }

    return result_msg;
}
