#pragma once
#include "Context.h"
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
    Context ctx_;
    std::thread worker_;
};

}

