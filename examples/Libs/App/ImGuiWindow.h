#pragma once
#include "App.h"
#include <imgui.h>

namespace qe::Examples
{

class ImGuiWindow : public ModuleBase
{
public:
    virtual bool CreateModule(int& width, int& height, std::string& title) = 0;
    virtual void RenderModule() = 0;
    virtual void CloseModule() = 0;

    // ModuleBase interface
protected:
    bool Create() override;
    void Loop() override;
    void Destroy() override;
private:
    int width_ = 0;
    int height_ = 0;
};


}
