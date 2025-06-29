#include <App.h>
#include <Audio.h>
#include "Context.h"
#include "ControlPanel.h"
#include "Display.h"
#include "Speaker.h"
#include "Computer.h"

namespace qe::Examples::AppleII
{

class AppleIIApp : public IApp
{
public:
    // IApp interface
public:
    void Init()
    {
        ctx_.controlPanel = MakePtr<ControlPanel>();
        ctx_.display = MakePtr<Display>();
        ctx_.speaker = MakePtr<Speaker>();
        ctx_.computer = MakePtr<Computer>();

        ctx_.controlPanel->RunModule(ctx_);
        ctx_.speaker->RunModule(ctx_);
        ctx_.display->RunModule(ctx_);
        ctx_.computer->RunModule(ctx_);
    }
    void Draw()
    {
        ctx_.controlPanel->Draw();
        ctx_.display->Draw();
    }
    void ProcessOnMinimize()
    {

    }
    void CustomRender()
    {
        ctx_.display->Render();
    }
    void Deinit()
    {
        ctx_.computer->DestroyModule();
        ctx_.display->DestroyModule();
        ctx_.speaker->DestroyModule();
        ctx_.controlPanel->DestroyModule();
    }

private:
    Context ctx_;
};

int Main()
{
    auto app = MakePtr<AppleIIApp>();
    return Program::Run(app);
}

}

int main()
{
    //return main2();
    return qe::Examples::AppleII::Main();
}
