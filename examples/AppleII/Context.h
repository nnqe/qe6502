#pragma once
#include <Memory.h>

namespace qe::Examples::AppleII
{

class ControlPanel;
class Display;
class Speaker;
class Computer;

struct Context
{
    Ptr<ControlPanel> controlPanel;
    Ptr<Display> display;
    Ptr<Speaker> speaker;
    Ptr<Computer> computer;
};

}
