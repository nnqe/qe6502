#include <qe_6502.h>

const char* test_klaus2m5(uint8_t cpu_model,
                          uint8_t* memory,
                          uint16_t success_address,
                          uint64_t expected_cycles,
                          qe_bool* result)
{
    *result = qe_false;
    qe6502_t cpu;

    memory[0xFFFC] = 0x00;
    memory[0xFFFD] = 0x04;
    qe6502_cycle_t cycle = qe6502_power_on(&cpu, cpu_model);
    if (!qe6502_ok(&cpu))
    {
        return "CPU power on error!";
    }

    // booting
    while(!qe6502_started(&cpu))
    {
        uint16_t address = qe6502_address(&cpu);
        qe_bool is_read = qe6502_needs_data(&cpu);
        uint8_t data = is_read ? memory[address] : qe6502_data(&cpu);
        if (is_read)
        {
            qe6502_feed_data(&cpu, data);
        }
        else
        {
            memory[address] = data;
        }
        cycle = cycle.execute(&cpu);
    }
    if (!qe6502_ok(&cpu))
    {
        return "CPU boot error";
    }

    uint64_t cycles = 0;
    while(qe6502_ok(&cpu))
    {
        uint16_t address = qe6502_address(&cpu);
        if (address == success_address)
        {
            if (expected_cycles != cycles)
            {
                return "Success addres reached, but expected cycles differs!";
            }
            else
            {
                *result = qe_true;
                return "OK";
            }
        }
        qe_bool is_read = qe6502_needs_data(&cpu);
        uint8_t data = is_read ? memory[address] : qe6502_data(&cpu);
        if (is_read)
        {
            qe6502_feed_data(&cpu, data);
        }
        else
        {
            memory[address] = data;
        }
        cycle = cycle.execute(&cpu);

        if (qe6502_instr_done(&cpu))
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

