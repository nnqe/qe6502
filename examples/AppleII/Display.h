#pragma once
#include <App.h>
#include "Context.h"

namespace qe::Examples::AppleII
{

class Display : public ModuleBase
{
public:
    void SetContext(Context ctx);
    // ModuleBase interface
public:
    bool Create() override;
    void Loop() override;
    void Destroy() override;

private:
    Context ctx_;
};

}

