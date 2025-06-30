#include <qe_appleIIhelpers.h>

static const uint64_t nanos_per_65536_clocks = 64079653ULL;

QE_API_IMPL
void qeaii_to_rgb(const qeaii_frame_t *frame, uint8_t *rgb_frame)
{
    for(unsigned i = 0; i < qeaii_width * qeaii_height / qeaii_pixels_per_clock; i++)
    {
        uint8_t pixels = frame->bitmap[i];
        for(int i = 0; i < 7; i++)
        {
            if (pixels & 1)
            {
                *rgb_frame++ = 10;
                *rgb_frame++ = 220;
                *rgb_frame++ = 10;
            }
            else
            {
                *rgb_frame++ = 5;
                *rgb_frame++ = 15;
                *rgb_frame++ = 5;
            }
            pixels >>= 1;
        }
    }
}

QE_API_IMPL
void qeaii_to_audio_samples(const qeaii_speaker_frame_t* frame,
                                    int16_t* output, unsigned output_size)
{
    float cycles = frame->end_cycle - frame->start_cycle + 1.0;
    float samples_per_cycle = (float)(output_size) / (float)(cycles);

    uint8_t speaker = frame->speaker_state > 0 ? 0 : 255;
    unsigned tick = 0;
    unsigned next = output_size;
    float next_coeff = 0.0;
    if (frame->tick_count > 0)
    {
        int asd = 34;
    }

    if (tick < frame->tick_count)
    {
        next_coeff = (float)(frame->ticks[tick]) * samples_per_cycle;
        next = next_coeff;
        next_coeff = next + 1 - next_coeff;
    }
    for(unsigned i = 0; i < output_size; i++)
    {
        if (i < next)
        {
            output[i] = speaker;
        }
        else
        {
            output[i] = speaker;
            while(i == next)
            {
                if (speaker > 0)
                {
                    // decrease
                    speaker = 0;
                    output[i] -= (float)(output[i]) * next_coeff;
                }
                else
                {
                    // increase
                    speaker = 255;
                    output[i] += (float)(255 - output[i]) * next_coeff;
                }
                tick++;
                if (tick < frame->tick_count)
                {
                    next_coeff = (float)(frame->ticks[tick]) * samples_per_cycle;
                    next = next_coeff;
                    next_coeff = next + 1 - next_coeff;
                }
                else
                {
                    next = output_size;
                }
            }
        }

        output[i] -= 0x80;
        output[i] *= 255;
    }
}

QE_API_IMPL
uint64_t qeaii_to_nanos(uint32_t clocks)
{
    return ((uint64_t)(clocks) * nanos_per_65536_clocks) / 65536;
}
