#pragma once
#include <App.h>
#include <qe_appleII.h>
#include "Context.h"

namespace qe::Examples::AppleII
{

class Computer : public ModuleBase
{
public:
    void SetContext(Context ctx);

    void Pause();
    void Resume();
    void Boot(std::vector<uint8_t> memory,
              std::vector<uint8_t> videoRom,
              std::vector<uint8_t> diskImage = {});
    void Shutdown();

    // ModuleBase interface
public:
    bool Create();
    void Loop();
    void Destroy();

private:

    enum class State
    {
        eFree,
        ePause,
        eRun,
        eRunFast
    };

    void FreeLoop();
    void PauseLoop();
    void RunLoop();
    void RunFastLoop();

    void WaitForStateChanged();

    inline bool ChangeRequested() const { return state_ != stateRequest_; }

    std::atomic<State> state_ = State::eFree;
    std::atomic<State> stateRequest_ = State::eFree;

    Context ctx_;
    Ptr<qeaii_t> appleII_;
    Ptr<qeaii_bootstrap_t> bootstrap_;
    std::thread::id cpuThread_;
};

}

