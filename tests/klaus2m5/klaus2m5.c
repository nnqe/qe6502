#include <qe6502/qe6502.h>
#include <stdalign.h>
#include <string.h>


const char* test_klaus2m5(uint8_t cpu_model,
                          uint8_t* memory,
                          uint16_t success_address,
                          uint64_t expected_cycles,
                          uint8_t  reset_cpu_each_cycle,
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
    int reset_counter = 0;
    int instr_cycle_counter = 0;

    uint64_t stable_state = qe6502_dump(cpu_ptr);
    const char* result_msg = "CPU Error";

    if (reset_cpu_each_cycle)
    {
        qe6502_pause_logger();
    }
    while(qe6502_ok(cpu_ptr))
    {
        if (reset_cpu_each_cycle)
        {
            if (reset_counter == instr_cycle_counter)
            {
                reset_counter++;

                // reset cpu
                qe6502_pause_logger();
                memset(cpu_ptr, 0, sizeof(cpu));
                qe6502_cpu_power_on(cpu_ptr, cpu_model);

                qe6502_recover(cpu_ptr, stable_state);
                qe6502_resume_logger();

                instr_cycle_counter = 0;
            }
        }
        uint16_t address = qe6502_address(cpu_ptr);
        if (address == success_address)
        {
            if (expected_cycles != cycles)
            {
                result_msg = "Success addres reached, but expected cycles differs!";
                break;
            }
            else
            {
                *result = 1;
                result_msg = "OK";
                break;
            }
        }
        if ( qe6502_needs_data(cpu_ptr) )
        {
            qe6502_feed_data(cpu_ptr, memory[address]);
        }
        else
        {
            memory[address] = qe6502_read_data(cpu_ptr);
        }

        qe6502_cpu_tick(cpu_ptr);

        if (qe6502_is_instr_done(cpu_ptr))
        {
            stable_state = qe6502_dump(cpu_ptr);
            instr_cycle_counter = 0;
            reset_counter = 0;
            cycles++;
            if (cycles > 2 * expected_cycles)
            {
                result_msg = "Test fail, takes too many cycles!";
                break;
            }
        }
        else
        {
            instr_cycle_counter++;
        }
    }

    return result_msg;
}
