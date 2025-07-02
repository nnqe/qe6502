#pragma once

#include "Context.h"
#include <qe_appleII.h>
#include <map>
#include <atomic>

namespace qe::Examples::AppleII
{

class Keyboard
{
public:
    void RunModule(Context ctx);
    void DestroyModule();
    void UpdateModule();

    uint8_t GetKey();
    bool HasBreakKey();
private:
    void CreateKeys();

    Context ctx_;
    std::map<int, uint8_t> normalMode_;
    std::map<int, uint8_t> shiftMode_;
    std::map<int, uint8_t> ctrlMode_;
    std::atomic<uint8_t> key_;
    std::atomic<bool> breakKey_;
};
}
