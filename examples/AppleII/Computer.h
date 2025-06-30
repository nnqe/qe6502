#pragma once
#include "Context.h"
#include <App.h>
#include <Stopwatch.h>
#include <qe_appleII.h>
#include <thread>
#include <vector>

namespace qe::Examples::AppleII
{

class Computer
{
public:
    void RunModule(Context ctx);
    void DestroyModule();

    bool IsOn() const;
    bool IsPaused() const;
    bool IsRunning() const;
    bool IsTurboSpeed();
    void Pause();
    void Resume();
    void Shutdown();
    void TurboSpeed();
    void NormalSpeed();
    void Boot(std::vector<uint8_t> memory,
              std::vector<uint8_t> videoRom,
              std::vector<uint8_t> diskImage = {});

private:
    void Loop();
    void PauseLoop();
    void RunLoop();
    void FastRunLoop();

    void WaitForStateChanged();
    inline bool ChangeRequested() const { return state_ != stateRequest_; }

    void ResetSleepPolicy();
    void SleepPolicy(uint64_t executedInstructions, bool hasNewFrame);

    enum class State
    {
        eOff,
        ePause,
        eRunning,
        eTurboRunning
    };
    std::atomic<State> state_ = State::eOff;
    std::atomic<State> stateRequest_ = State::eOff;

    Context ctx_;
    Ptr<qeaii_t> appleII_;
    Ptr<qeaii_bootstrap_t> bootstrap_;
    std::thread::id cpuThread_;

    Stopwatch sleepPolicySw_;
    uint64_t sleepPolicyClocks_;
    std::thread worker_;
};

}

