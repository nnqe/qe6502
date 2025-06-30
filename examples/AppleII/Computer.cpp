#include "Computer.h"
#include "Display.h"
#include "Speaker.h"
#include <qe_appleIIhelpers.h>
#include <algorithm>
#include <App.h>

namespace qe::Examples::AppleII
{

void Computer::RunModule(Context ctx)
{
    ctx_ = ctx;
    worker_ = std::thread([this](){Loop();});
}

void Computer::DestroyModule()
{
    if (worker_.joinable())
    {
        worker_.join();
    }
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
    ctx_.display->ClearDisplay();
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

void Computer::Loop()
{
    cpuThread_ = std::this_thread::get_id();
    while(!Program::Ctx().done)
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

void Computer::PauseLoop()
{
    while(!Program::Ctx().done && !ChangeRequested())
    {
        std::this_thread::sleep_for(5ms);
    }
}

void Computer::RunLoop()
{
    static Clock::duration frameDuration = Clock::Millis(8);
    static uint32_t s_cyclesPerFrame = qeaii_to_cycles(frameDuration.count());
    auto& display = *ctx_.display;
    auto timePoint = Clock::now() + Clock::Millis(8);
    while(!Program::Ctx().done && !ChangeRequested())
    {
        uint32_t cyclesLeft = appleII_->driveII.spinning ? s_cyclesPerFrame * 100 : s_cyclesPerFrame;
        while(cyclesLeft)
        {
            cyclesLeft = qeaii_run(appleII_.get(), cyclesLeft);
            if (qeaii_frame_ready(appleII_.get()) &&
                display.IsReadyForRawFrame())
            {
                // push this frame
                auto frame = qeaii_frame(appleII_.get());
                display.NewRawFrame(frame);
            }
        }
        while(!ctx_.speaker->IsReadyForRawFrame())
        {
            // not expected!
            std::this_thread::sleep_for(Clock::Micros(500));
        }
        auto newRawAudio = qeaii_speaker_frame(appleII_.get());
        ctx_.speaker->NewRawFrame(newRawAudio, timePoint, frameDuration);
        timePoint += Clock::Millis(8);
        auto now = Clock::now();
        if ( now >= timePoint )
        {
            // we are too busy
            timePoint = now + Clock::Micros(100);
        }
        else
        {
            std::this_thread::sleep_for( (timePoint - now) / 2 );
        }
    }
}

void Computer::FastRunLoop()
{
    static Clock::duration s_passDuration = Clock::Millis(16);
    static uint32_t s_cyclesPerPass = qeaii_to_cycles(s_passDuration.count());
    auto& display = *ctx_.display;
    auto timePoint = Clock::now() + s_passDuration;
    while(!Program::Ctx().done && !ChangeRequested())
    {
        uint32_t cyclesLeft = appleII_->driveII.spinning ? s_cyclesPerPass * 100 : s_cyclesPerPass * 8;
        while(cyclesLeft)
        {
            cyclesLeft = qeaii_run(appleII_.get(), cyclesLeft);
            if (qeaii_frame_ready(appleII_.get()) &&
                display.IsReadyForRawFrame())
            {
                // push this frame
                auto frame = qeaii_frame(appleII_.get());
                display.NewRawFrame(frame);
            }
        }
        while(!ctx_.speaker->IsReadyForRawFrame())
        {
            // not expected!
            std::this_thread::sleep_for(Clock::Micros(500));
        }
        auto newRawAudio = qeaii_speaker_frame(appleII_.get());
        ctx_.speaker->NewRawFrame(newRawAudio, timePoint, s_passDuration);
        timePoint += s_passDuration;
        auto now = Clock::now();
        if ( now >= timePoint )
        {
            // we are too busy
            timePoint = now + Clock::Micros(100);
        }
        else
        {
            std::this_thread::sleep_for( (timePoint - now) / 2 );
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

}
