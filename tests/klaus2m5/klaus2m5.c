#include <qe6502/qe6502.h>
#include <stdalign.h>
#include <string.h>

void pause_logger(void);
void resume_logger(void);

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
    qe6502_tick_t tick = qe6502_reset(cpu_ptr, cpu_model);

    if (QE6502_IS_NOT_OK(tick))
    {
        return "CPU power on error!";
    }

    // booting
    while(QE6502_IS_STARTING(tick) && QE6502_IS_OK(tick))
    {
        uint16_t address = QE6502_ADDRESS(tick);
        uint8_t is_write = QE6502_IS_WRITING(tick);
        uint8_t data = is_write ? QE6502_DATA(tick) : memory[address];
        if (!is_write)
        {
            memory[address] = data;
        }
        tick = qe6502_tick(cpu_ptr, data);
    }
    if (QE6502_IS_NOT_OK(tick))
    {
        return "CPU boot error";
    }

    uint64_t cycles = 0;
    int reset_counter = 0;
    int instr_cycle_counter = 0;

    qe6502_state_t stable_state = qe6502_dump_state(cpu_ptr);
    const char* result_msg = "CPU Error";

    while(QE6502_IS_OK(tick))
    {
        if (reset_cpu_each_cycle)
        {
            if (reset_counter == instr_cycle_counter)
            {
                reset_counter++;

                // reset cpu
                pause_logger();
                memset(cpu_ptr, 0, sizeof(cpu));
                qe6502_reset(cpu_ptr, cpu_model);

                tick = qe6502_recover(cpu_ptr, stable_state);
                resume_logger();

                instr_cycle_counter = 0;
            }
        }
        uint16_t address = QE6502_ADDRESS(tick);
        uint8_t is_write = QE6502_IS_WRITING(tick);
        uint8_t data = is_write ? QE6502_DATA(tick) : memory[address];

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
        if ( is_write )
        {
            memory[address] = data;
        }
        else
        {

            data = memory[address];
        }

        tick = qe6502_tick(cpu_ptr, data);

        if (QE6502_IS_INSTR_DONE(tick))
        {
            stable_state = qe6502_dump_state(cpu_ptr);
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
