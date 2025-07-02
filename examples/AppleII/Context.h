#pragma once
#include <Memory.h>

namespace qe::Examples::AppleII
{

class ControlPanel;
class Display;
class Speaker;
class Keyboard;
class Computer;

struct Context
{
    Ptr<ControlPanel> controlPanel;
    Ptr<Display> display;
    Ptr<Speaker> speaker;
    Ptr<Keyboard> keyboard;
    Ptr<Computer> computer;
};

}
