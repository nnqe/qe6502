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
    frameConsumed_.test_and_set();
    worker_ = std::thread([this](){AudioLoop();});
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

void Speaker::NewRawFrame(qeaii_speaker_frame_t *rawFrame, Clock::time_point start, Clock::duration duration)
{
    if (HasReadyRawFrame())
    {
        fmt::println("New audio frame cannot be set. Possible race-condititon. please fix it !");
        exit(-111);
    }
    rawFrame_ = rawFrame;
    nextFrameStart_ = start;
    nextFrameDuration_ = duration;
    if (!Program::Ctx().done)
    {
        frameConsumed_.clear();
    }
}

void Speaker::AudioLoop()
{
    static int maxTicks = 0;
    while(!Program::Ctx().done)
    {
        if (HasReadyRawFrame())
        {
            if (rawFrame_->tick_count == 0)
            {
                // empty frame
                frameConsumed_.test_and_set();
                continue;
            }

            if (rawFrame_->tick_count > maxTicks)
            {
                maxTicks = rawFrame_->tick_count;
                fmt::println("Max ticks: {}", maxTicks);
            }

            uint32_t samplesQueued = SDL_GetQueuedAudioSize( Program::Ctx().audioDeviceId );

            // process it
            size_t payloadStart = 0;
            double seconds = Clock::ToSeconds(nextFrameDuration_);
            size_t samples = seconds * Program::Ctx().audioSpecs.freq;

            auto now = Clock::now();
            if (nextFrameStart_ > now)
            {
                size_t emptySamples = Clock::ToSeconds(nextFrameStart_ - now) * Program::Ctx().audioSpecs.freq;
                if (samplesQueued < emptySamples)
                {
                    emptySamples -= samplesQueued;
                }
                else
                {
                    emptySamples = 0;
                }
                samples += emptySamples;
                payloadStart = emptySamples;
            }
            audioBuff_.resize(samples);
            auto silence = Program::Ctx().audioSpecs.silence;
            for(size_t i = 0; i < payloadStart; i++)
            {
                audioBuff_[i] = silence;
            }
            qeaii_to_audio_samples(rawFrame_, audioBuff_.data() + payloadStart, samples - payloadStart);

            SDL_QueueAudio(Program::Ctx().audioDeviceId,
                           audioBuff_.data(),
                           audioBuff_.size() * sizeof(int16_t));
            frameConsumed_.test_and_set();
        }
        std::this_thread::sleep_for(1ms);
    }
    frameConsumed_.test_and_set();
}


}
