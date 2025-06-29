#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <SpinLock.h>
#include <memory>

#include <fmt/format.h>
struct GLFWwindow;

namespace qe::Examples
{
using namespace std::chrono;
template<typename T>
using Ptr = std::shared_ptr<T>;
template<typename T, typename ... Args>
inline Ptr<T> MakePtr(Args&&...args){return std::make_shared<T>(std::forward<Args>(args)...);}

enum class EvType { Key, Char, MouseBtn, CursorPos, Scroll };
struct Event
{
    EvType type;
    union
    {
        struct { int key, scancode, action, mods; } key;
        struct { unsigned int codepoint; } ch;
        struct { int button, action, mods; } mouse;
        struct { double x, y; } cursor;
        struct { double xoff, yoff; } scroll;
    };
};

class App;

class ModuleBase
{
public:
    using Handle = GLFWwindow*;
    virtual ~ModuleBase();

    virtual bool Create() = 0;
    virtual void Loop() = 0;
    virtual void Destroy() = 0;

protected:
    bool CreateWindow(int width, int height, const char* title);
    void CloseWindow();
    Handle GetWindowHandle() const;
    const std::vector<Event>& GetEvents();
    bool ShouldExit() const;
    void ExitRequest();

private:
    void StartLoop();
    void WaitExit();
    void PushEvent(Event ev);

    std::thread mainThread_;
    std::atomic<bool> shoudExit_;
    SpinLock lock_;
    Handle windowHandle_ = nullptr;
    std::vector<Event> eventQueus_[2];
    size_t clientEventQueue_ = 0;
    size_t freeEventQueue_ = 1;
    friend class App;
};

class App
{
public:
    static void AddModule(Ptr<ModuleBase> module);
    static bool Run();

    static void KeyCallback(ModuleBase::Handle w,int k,int s,int a,int m);
    static void CharCallback(ModuleBase::Handle w,unsigned int cp);
    static void MouseBtnCallback(ModuleBase::Handle w,int b,int a,int m);
    static void CursorPosCallback(ModuleBase::Handle w,double x,double y);
    static void ScrollCallback(ModuleBase::Handle w,double xo,double yo);
};

}
