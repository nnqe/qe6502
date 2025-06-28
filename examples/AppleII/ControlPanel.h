#pragma once
#include <ImGuiWindow.h>
#include "Context.h"

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
    Context ctx_;
};

}
