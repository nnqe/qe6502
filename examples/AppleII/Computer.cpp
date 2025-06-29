#include "Computer.h"
#include "Display.h"
#include <Stopwatch.h>
#include <algorithm>

namespace qe::Examples::AppleII
{

void Computer::SetContext(Context ctx)
{
    ctx_ = ctx;
}

void Computer::Pause()
{

}

void Computer::Resume()
{

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
    stateRequest_.store(State::eRun);
    WaitForStateChanged();
}

void Computer::Shutdown()
{
    stateRequest_.store(State::eFree);
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
        case State::eFree:
            FreeLoop();
            break;
        case State::ePause:
            PauseLoop();
            break;
        case State::eRun:
            RunLoop();
            break;
        case State::eRunFast:
            RunFastLoop();
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

void Computer::FreeLoop()
{
    while(!ShouldExit() && !ChangeRequested())
    {
        std::this_thread::sleep_for(5ms);
    }
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
    uint64_t instructions = 0;
    Stopwatch sw;
    while(!ShouldExit() && !ChangeRequested())
    {
        instructions += qeaii_run(appleII_.get(), 10'000);
        if (qeaii_frame_ready(appleII_.get()) &&
            display.IsReadyForRawFrame())
        {
            auto frame = qeaii_frame(appleII_.get());
            display.NewRawFrame(frame);
        }
        // AppleII is ~1Mhz
        auto micros = sw.Measure().count() / 1000;
        int a = sizeof(micros);
        uint64_t sleepFor = instructions - micros;
        sleepFor = std::clamp(sleepFor, 1'000ul, 10'000ul);
        //std::this_thread::sleep_for(Clock::Micros(sleepFor));
    }
}

void Computer::RunFastLoop()
{

}

void Computer::WaitForStateChanged()
{
    if (std::this_thread::get_id() == cpuThread_)
    {
        fmt::println("Unexpected call from cpu thread");
        exit(-4);
    }

    std::this_thread::sleep_for(6ms);
    for(int i = 0; i < 4 && (state_ != stateRequest_); i++)
    {
        std::this_thread::sleep_for(60ms);
    }

    if (state_ != stateRequest_)
    {
        fmt::println("CPU too busy, deadlock detected!");
        exit(-5);
    }
}

}
