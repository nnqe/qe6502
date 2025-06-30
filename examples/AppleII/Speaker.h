#pragma once
#include "Context.h"
#include <Clock.h>
#include <qe_appleII.h>
#include <thread>
#include <vector>

namespace qe::Examples::AppleII
{

class Speaker
{
public:
    void RunModule(Context ctx);
    void DestroyModule();

    bool IsReadyForRawFrame() const;
    bool HasReadyRawFrame() const;
    void NewRawFrame(qeaii_speaker_frame_t* rawFrame, Clock::time_point start, Clock::duration duration);

private:

    void AudioLoop();

    std::vector<int16_t> audioBuff_;
    std::atomic_flag frameConsumed_;
    qeaii_speaker_frame_t* rawFrame_ = nullptr;
    Clock::time_point nextFrameStart_;
    Clock::duration nextFrameDuration_;
    Context ctx_;
    std::thread worker_;
};

}

