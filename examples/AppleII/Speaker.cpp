#include "Speaker.h"
#include <CountdownTimer.h>
#include <thread>
#include <App.h>
#include <SDL.h>
#include <fmt/format.h>

namespace qe::Examples::AppleII
{

struct AudioData
{
    double Phase;            // current phase of the sine wave
    double PhaseIncrement;   // phase increment per sample
    uint32_t SamplesPlayed;    // number of samples already generated
    uint32_t MaxSamples;       // total samples for 3 seconds
};

AudioData Data{};

void AudioCallback(void* UserData, Uint8* Stream, int Len)
{
    AudioData* Data = static_cast<AudioData*>(UserData);
    Sint16* Buffer = reinterpret_cast<Sint16*>(Stream);
    int SamplesToWrite = Len / sizeof(Sint16);

    for (int i = 0; i < SamplesToWrite; ++i) {
        if (Data->SamplesPlayed >= Data->MaxSamples) {
            // After 3 seconds, output silence
            Buffer[i] = 0;
        } else {
            // generate sine wave sample
            double Sample = std::sin(Data->Phase) * 28000;
            Buffer[i] = static_cast<Sint16>(Sample);

            // advance phase and sample count
            Data->Phase += Data->PhaseIncrement;
            if (Data->Phase >= M_PI * 2.0) {
                Data->Phase -= M_PI * 2.0;
            }
            ++Data->SamplesPlayed;
        }
    }
}

void Speaker::RunModule(Context ctx)
{
    // Desired audio spec
    SDL_AudioSpec DesiredSpec;
    SDL_zero(DesiredSpec);
    DesiredSpec.freq = 48000;             // sample rate
    DesiredSpec.format = AUDIO_S16SYS;    // 16-bit signed
    DesiredSpec.channels = 1;             // mono
    DesiredSpec.samples = 1024;           // buffer size
    DesiredSpec.callback = AudioCallback; // our callback

    Data.Phase = 0.0;
        double Frequency = 440.0; // A4 tone
        Data.PhaseIncrement = 2.0 * M_PI * Frequency / DesiredSpec.freq;
        Data.SamplesPlayed = 0;
        Data.MaxSamples = DesiredSpec.freq * 3; // 3 seconds

        DesiredSpec.userdata = &Data;

        // Open audio device
        SDL_AudioSpec ObtainedSpec;
        if (SDL_OpenAudio(&DesiredSpec, &ObtainedSpec) < 0) {
            fmt::println("SDL_OpenAudio Error: {}", SDL_GetError());
            SDL_Quit();
            return;
        }

        // Start playback
        SDL_PauseAudio(0);

    worker_ = std::thread([this](){
        std::this_thread::sleep_for(1s);
    });
}

void Speaker::DestroyModule()
{
    SDL_CloseAudio();

    if (worker_.joinable())
    {
        worker_.join();
    }
}


}
