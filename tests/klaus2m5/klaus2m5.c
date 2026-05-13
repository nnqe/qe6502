#include <qe6502/qe6502.h>
#include <stdalign.h>

const char* test_klaus2m5(uint8_t cpu_model,
                          uint8_t* memory,
                          uint16_t success_address,
                          uint64_t expected_cycles,
                          uint8_t* result)
{
    *result = 0;
    qe6502_cpu_t cpu;
    qe6502_cpu_t* cpu_ptr = &cpu;

    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;
    qe6502_cpu_power_on(cpu_ptr, cpu_model);

    if (!qe6502_ok(cpu_ptr))
    {
        return "CPU power on error!";
    }

    // booting
    while(!qe6502_is_started(cpu_ptr))
    {
        uint16_t address = qe6502_address(cpu_ptr);
        uint8_t is_read = qe6502_needs_data(cpu_ptr);
        uint8_t data = is_read ? memory[address] : qe6502_read_data(cpu_ptr);
        if (is_read)
        {
            qe6502_feed_data(cpu_ptr, data);
        }
        else
        {
            memory[address] = data;
        }
        qe6502_cpu_tick(cpu_ptr);
    }
    if (!qe6502_ok(cpu_ptr))
    {
        return "CPU boot error";
    }

    uint64_t cycles = 0;
    while(qe6502_ok(cpu_ptr))
    {
        uint16_t address = qe6502_address(cpu_ptr);
        if (address == success_address)
        {
            if (expected_cycles != cycles)
            {
                return "Success addres reached, but expected cycles differs!";
            }
            else
            {
                *result = 1;
                return "OK";
            }
        }
        uint8_t is_read = qe6502_needs_data(cpu_ptr);
        uint8_t data = is_read ? memory[address] : qe6502_read_data(cpu_ptr);
        if (is_read)
        {
            qe6502_feed_data(cpu_ptr, data);
        }
        else
        {
            memory[address] = data;
        }
        qe6502_cpu_tick(cpu_ptr);

        if (qe6502_is_instr_done(cpu_ptr))
        {
            cycles++;
            if (cycles > 2 * expected_cycles)
            {
                return "Test fail, takes too many cycles!";
            }
        }
    }
    return "CPU Error";
}
