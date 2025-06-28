#pragma once
#include <App.h>
#include <qe_appleII.h>
#include "Context.h"

namespace qe::Examples::AppleII
{

class Speaker : public ModuleBase
{
public:
    void SetContext(Context ctx);
    // ModuleBase interface
public:
    bool Create();
    void Loop();
    void Destroy();

private:
    void GenerateSoundFrame(uint8_t* output, uint32_t frameSize);
    Context ctx_;
    std::atomic<bool> testPlay_{true};
};

}

