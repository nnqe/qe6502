#include <App.h>
#include <Audio.h>
#include "Context.h"
#include "ControlPanel.h"
#include "Display.h"
#include "Speaker.h"
#include "Computer.h"

namespace qe::Examples::AppleII
{

int Main()
{
    Context ctx;
    ctx.controlPanel = std::make_shared<ControlPanel>();
    ctx.display = std::make_shared<Display>();
    ctx.speaker = std::make_shared<Speaker>();
    ctx.computer = std::make_shared<Computer>();

    ctx.controlPanel->SetContext(ctx);
    ctx.display->SetContext(ctx);
    ctx.speaker->SetContext(ctx);
    ctx.computer->SetContext(ctx);

    App::AddModule(ctx.controlPanel);
    App::AddModule(ctx.display);
    App::AddModule(ctx.speaker);
    App::AddModule(ctx.computer);

    return App::Run() ? 0 : -1;
}

}

int main()
{
    return qe::Examples::AppleII::Main();
}
