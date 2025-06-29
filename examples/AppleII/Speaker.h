#pragma once
#include "Context.h"
#include <App.h>
#include <qe_appleII.h>
#include <thread>

namespace qe::Examples::AppleII
{

class Speaker
{
public:
    void RunModule(Context ctx);
    void DestroyModule();

private:
    void GenerateSoundFrame(uint8_t* output, uint32_t frameSize);
    Context ctx_;
    std::atomic<bool> testPlay_{true};
    std::thread worker_;
};

}

