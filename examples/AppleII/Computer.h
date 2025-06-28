#pragma once
#include <App.h>
#include <qe_appleII.h>
#include "Context.h"

namespace qe::Examples::AppleII
{

class Computer : public ModuleBase
{
public:
    void SetContext(Context ctx);
    // ModuleBase interface
public:
    bool Create();
    void Loop();
    void Destroy();

private:
    Context ctx_;
    aii_appleII_t appleII_;
};

}

