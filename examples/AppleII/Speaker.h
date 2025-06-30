#pragma once
#include "Context.h"
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
    void NewRawFrame(qeaii_speaker_frame_t* rawFrame);

private:

    void AudioLoop();

    std::vector<int16_t> audioBuff_;
    std::atomic_flag frameConsumed_;
    qeaii_speaker_frame_t* rawFrame_ = nullptr;
    Context ctx_;
    std::thread worker_;
};

}

