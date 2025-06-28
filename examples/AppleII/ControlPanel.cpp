#include "ControlPanel.h"

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
    if(ImGui::Button("Close")) ExitRequest();
    ImGui::End();
}

void ControlPanel::CloseModule()
{

}

}
