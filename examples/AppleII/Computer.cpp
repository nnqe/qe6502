#include "Computer.h"

namespace qe::Examples::AppleII
{

void Computer::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool Computer::Create()
{
    return true;
}

void Computer::Loop()
{
    while(!ShouldExit())
    {
        std::this_thread::sleep_for(16ms);
    }
}

void Computer::Destroy()
{

}

}
