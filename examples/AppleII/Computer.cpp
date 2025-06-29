#include "Computer.h"
#include "Display.h"
#include <algorithm>

namespace qe::Examples::AppleII
{

void Computer::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool Computer::IsOn() const
{
    return state_ != State::eOff;
}

bool Computer::IsPaused() const
{
    return state_ == State::ePause;
}

bool Computer::IsRunning() const
{
    return state_ != State::eOff && state_ != State::ePause;
}

bool Computer::IsTurboSpeed()
{
    return state_.load() == State::eTurboRunning;
}

void Computer::Pause()
{
    if (state_ != State::eOff)
    {
        stateRequest_.store(State::ePause);
        WaitForStateChanged();
    }
}

void Computer::Resume()
{
    if (state_.load() == State::ePause)
    {
        stateRequest_.store(State::eRunning);
        WaitForStateChanged();
    }
}

void Computer::Shutdown()
{
    stateRequest_.store(State::eOff);
    WaitForStateChanged();
}

void Computer::TurboSpeed()
{
    if (state_ != State::eOff)
    {
        stateRequest_.store(State::eTurboRunning);
        WaitForStateChanged();
    }
}

void Computer::NormalSpeed()
{
    if (state_ != State::eOff)
    {
        stateRequest_.store(State::eRunning);
        WaitForStateChanged();
    }
}

void Computer::Boot(std::vector<uint8_t> memory,
                    std::vector<uint8_t> videoRom,
                    std::vector<uint8_t> diskImage)
{
    Shutdown();

    auto bootstrapPtr = MakePtr<qeaii_bootstrap_t>();
    auto& bootstrap = *bootstrapPtr;

    memset(&bootstrap, 0, sizeof(bootstrap));

    if (!diskImage.empty() &&
        diskImage.size() != sizeof(bootstrap.disk0.data))
    {
        fmt::println("Disk image error");
        exit(-1);
    }

    if (memory.size() != sizeof(bootstrap.mem) ||
        videoRom.size() != sizeof(bootstrap.font_rom))
    {
        fmt::println("Roms error");
        exit(-2);
    }

    memcpy(&bootstrap.mem, memory.data(), sizeof(bootstrap.mem));
    memcpy(&bootstrap.font_rom, videoRom.data(), sizeof(bootstrap.font_rom));

    bootstrap.mount_disk0 = false;
    bootstrap.disk0.changed = qe_false;
    bootstrap.disk0.readonly = qe_false;
    bootstrap.first_rom_address = 0xc000;

    if (diskImage.size() == sizeof(bootstrap.disk0.data))
    {
        memcpy(&bootstrap.disk0.data, diskImage.data(), sizeof(bootstrap.disk0.data));
        bootstrap.mount_disk0 = true;
    }

    appleII_ = MakePtr<qeaii_t>();
    if (!qeaii_power_on(appleII_.get() , bootstrapPtr.get()))
    {
        fmt::println("CPU error");
        exit(-3);
    }

    bootstrap_ = bootstrapPtr;
    stateRequest_.store(State::eRunning);
    WaitForStateChanged();
}

bool Computer::Create()
{
    return true;
}

void Computer::Loop()
{
    cpuThread_ = std::this_thread::get_id();
    while(!ShouldExit())
    {
        state_.store(stateRequest_);
        switch (state_.load())
        {
        case State::eOff:
            // Do nothing
            break;
        case State::ePause:
            PauseLoop();
            break;
        case State::eRunning:
            RunLoop();
            break;
        case State::eTurboRunning:
            FastRunLoop();
            break;
        default:
            break;
        }
        std::this_thread::sleep_for(16ms);
    }
}

void Computer::Destroy()
{

}

void Computer::PauseLoop()
{
    while(!ShouldExit() && !ChangeRequested())
    {
        std::this_thread::sleep_for(5ms);
    }
}

void Computer::RunLoop()
{
    auto& display = *ctx_.display;

    ResetSleepPolicy();
    while(!ShouldExit() && !ChangeRequested())
    {
        // Run for a while
        auto clocks = qeaii_run(appleII_.get(), 5'000);

        bool newFrame = false;
        if (qeaii_frame_ready(appleII_.get()) &&
            display.IsReadyForRawFrame())
        {
            auto frame = qeaii_frame(appleII_.get());
            display.NewRawFrame(frame);

            newFrame = true;
        }
        SleepPolicy(clocks, newFrame);
    }
}

void Computer::FastRunLoop()
{
    auto& display = *ctx_.display;
    uint64_t instructions = 0;
    while(!ShouldExit() && !ChangeRequested())
    {
        instructions += qeaii_run(appleII_.get(), 16'000);
        if (qeaii_frame_ready(appleII_.get()) &&
            display.IsReadyForRawFrame())
        {
            auto frame = qeaii_frame(appleII_.get());
            display.NewRawFrame(frame);
        }
    }
}

void Computer::WaitForStateChanged()
{
    if (std::this_thread::get_id() == cpuThread_)
    {
        fmt::println("Unexpected call from cpu thread");
        exit(-4);
    }

    for(int i = 0; i < 4 && (state_ != stateRequest_); i++)
    {
        std::this_thread::sleep_for(10ms);
    }

    if (state_ != stateRequest_)
    {
        fmt::println("CPU too busy, deadlock detected!");
        exit(-5);
    }
}

void Computer::ResetSleepPolicy()
{
    sleepPolicySw_.Reset();
    sleepPolicyClocks_ = 0;
}

void Computer::SleepPolicy(uint64_t clocks, bool hasNewFrame)
{
    sleepPolicyClocks_ += clocks;
    if (appleII_->driveII.spinning)
    {
        // do not sleep, AppleII is loading
        std::this_thread::yield();
        ResetSleepPolicy();
    }
    else
    {
        // AppleII is ~1Mhz
        auto micros = sleepPolicySw_.Measure().count() / 1000;
        uint64_t sleepFor = sleepPolicyClocks_ - micros;

        if (hasNewFrame)
        {
            sleepFor = std::clamp(sleepFor, 1'000ul, 60'000ul);
        }
        else
        {
            sleepFor = std::clamp(sleepFor, 1ul, 1'000ul);
        }
        std::this_thread::sleep_for(Clock::Micros(sleepFor));
    }
}

}
