#include "Speaker.h"
#include <Audio.h>
#include <CountdownTimer.h>
#include <thread>

namespace qe::Examples::AppleII
{

void Speaker::RunModule(Context ctx)
{
    Audio::Init(Audio::Format_u8,
                1,            // channels
                48'000,       // 48'000 sample rate / PCM rate
                256,          // frames per perio
                [this](void* output, uint32_t frameSize){
                    GenerateSoundFrame(static_cast<uint8_t*>(output), frameSize);
                },
                nullptr);

    worker_ = std::thread([this](){
        std::this_thread::sleep_for(3s);
        testPlay_.store(false);
    });
}

void Speaker::DestroyModule()
{
    if (worker_.joinable())
    {
        worker_.join();
    }
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
