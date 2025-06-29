#pragma once
#include "Context.h"
#include <App.h>
#include <Json.h>
#include <Fs.h>

namespace qe::Examples::AppleII
{

class ControlPanel
{
public:
    void RunModule(Context ctx);
    void DestroyModule();
    void Draw();
private:

    void ShowOpenFile();
    void ShowCpuCtrl();
    void ShowStartControl();
    void ShowStopControl();
    void ShowSpeedControl();
    void ShowPauseControl();

    void SaveUserspace();
    void LoadUserspace();

    static std::vector<uint8_t> LoadDiskette(Fs::Path file);
    void OnRun();

    Context ctx_;
    Json userspace_;
    Fs::Path diskImageFile_;
};

}
