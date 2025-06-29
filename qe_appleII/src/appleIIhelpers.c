#include <qe_appleIIhelpers.h>


void qeaii_frame_to_rgb(const qeaii_frame_t *frame, uint8_t *rgb_frame)
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
