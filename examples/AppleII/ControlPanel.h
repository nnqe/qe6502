#pragma once
#include <ImGuiWindow.h>
#include "Context.h"
#include <Json.h>
#include <Fs.h>

namespace qe::Examples::AppleII
{

class ControlPanel : public ImGuiWindow
{
public:
    void SetContext(Context ctx);
    // ImGuiWindow interface
public:
    bool CreateModule(int &width, int &height, std::string &title) override;
    void RenderModule() override;
    void CloseModule() override;
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
