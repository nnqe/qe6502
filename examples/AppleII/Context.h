#pragma once
#include <memory>

namespace qe::Examples::AppleII
{

class ControlPanel;
class Display;
class Speaker;
class Computer;

struct Context
{
    std::shared_ptr<ControlPanel> controlPanel;
    std::shared_ptr<Display> display;
    std::shared_ptr<Speaker> speaker;
    std::shared_ptr<Computer> computer;
};


}
