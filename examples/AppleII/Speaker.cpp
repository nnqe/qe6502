#include "Speaker.h"
#include <Audio.h>
#include <CountdownTimer.h>
namespace qe::Examples::AppleII
{

void Speaker::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool Speaker::Create()
{
    return
    Audio::Init(Audio::Format_u8,
                1,            // channels
                48'000,       // 48'000 sample rate / PCM rate
                256,          // frames per perio
                [this](void* output, uint32_t frameSize){
                    GenerateSoundFrame(static_cast<uint8_t*>(output), frameSize);
                },
              nullptr);
}

void Speaker::Loop()
{
    CountdownTimer t(1s);
    t.Start();
    while(!ShouldExit())
    {
        if (t.GetRemainingTime().count() <= 0)
        {
            testPlay_.store(false);
        }
        std::this_thread::sleep_for(16ms);
    }
}

void Speaker::Destroy()
{

}

void Speaker::GenerateSoundFrame(uint8_t *output, uint32_t frameSize)
{
    if (!testPlay_)
    {
        return;
    }
    for (uint32_t i = 0; i < frameSize; ++i)
    {
        output[i] = (i % 64) > 32  ? 255 : 0;
    }
}

}
