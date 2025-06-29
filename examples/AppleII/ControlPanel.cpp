#include "ControlPanel.h"
#include "Computer.h"
#include "roms/Roms.h"
#include <Dsk2Nib.h>

namespace qe::Examples::AppleII
{

void ControlPanel::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool ControlPanel::CreateModule(int &width, int &height, std::string &title)
{
    width = 800;
    height = 600;
    title = "AppleII Control Panel";
    return true;
}

void ControlPanel::RenderModule()
{
    ImGui::Begin("Demo");
    static int val = 0;
    ImGui::InputInt("Value", &val);
    if (ImGui::Button("Test Run"))
    {
        auto diskette = Fs::Dsk2Nib("/home/nikolay/ndn/Proj/bins/cpp-6502/Desktop-Release/assets/roms/Games/lode_runner.dsk");
        ctx_.computer->Boot(Roms::Get_AppleII_Plus_DiskII_Card5(),
                            Roms::Get_AppleII_Plus_VideoRom(),
                            diskette);
    }
    if(ImGui::Button("Close")) ExitRequest();
    ImGui::End();
}

void ControlPanel::CloseModule()
{

}

}
