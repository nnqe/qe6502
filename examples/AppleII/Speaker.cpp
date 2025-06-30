#include "Speaker.h"
#include <CountdownTimer.h>
#include <thread>
#include <App.h>
#include <SDL.h>
#include <fmt/format.h>
#include <qe_appleIIhelpers.h>

namespace qe::Examples::AppleII
{

void Speaker::RunModule(Context ctx)
{
    // // Desired audio spec
    // const double frequency = 440.0;
    // const int sampleRate   = Program::Ctx().audioSpecs.freq;
    // const int64_t totalSamples = sampleRate * 3;
    // std::vector<int16_t> buffer;
    // buffer.reserve(totalSamples);

    // double Phase = 0.0;
    // double PhaseInc = 2.0 * M_PI * frequency / sampleRate;
    // for (int64_t i = 0; i < totalSamples; ++i) {
    //     double value = std::sin(Phase) * 28000.0;
    //     buffer.push_back(static_cast<Sint16>(value));
    //     Phase += PhaseInc;
    //     if (Phase >= 2.0 * M_PI) Phase -= 2.0 * M_PI;
    // }

    // if (SDL_QueueAudio(Program::Ctx().audioDeviceId,
    //                    buffer.data(),
    //                    buffer.size() * sizeof(Sint16)) < 0)
    // {
    //     fmt::println("SDL_QueueAudio Error: {}", SDL_GetError());
    // }

    worker_ = std::thread([this](){AudioLoop();});

    frameConsumed_.test_and_set();
}

void Speaker::DestroyModule()
{
    if (worker_.joinable())
    {
        worker_.join();
    }
}

bool Speaker::IsReadyForRawFrame() const
{
    return frameConsumed_.test();
}

bool Speaker::HasReadyRawFrame() const
{
    return !frameConsumed_.test();
}

void Speaker::NewRawFrame(qeaii_speaker_frame_t *rawFrame)
{
    if (HasReadyRawFrame())
    {
        fmt::println("New audio frame cannot be set. Possible race-condititon. please fix it !");
        exit(-111);
    }
    rawFrame_ = rawFrame;
    frameConsumed_.clear();
}

void Speaker::AudioLoop()
{
    while(!Program::Ctx().done)
    {
        if (HasReadyRawFrame())
        {
            // process it
            if (rawFrame_->tick_count > 0)
            {
                uint64_t nanos = qeaii_to_nanos(rawFrame_->end_cycle - rawFrame_->start_cycle + 1);
                size_t samples = (Program::Ctx().audioSpecs.freq * nanos) / 1'000'000'000;
                audioBuff_.resize(samples);
                qeaii_to_audio_samples(rawFrame_, audioBuff_.data(), samples);

                SDL_QueueAudio(Program::Ctx().audioDeviceId,
                               audioBuff_.data(),
                               samples * sizeof(int16_t));
            }

            frameConsumed_.test_and_set();
        }
        std::this_thread::sleep_for(3ms);
    }
}


}
