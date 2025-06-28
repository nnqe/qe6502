#include "App.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <map>

namespace qe::Examples
{
using namespace std::chrono;


ModuleBase::~ModuleBase() = default;
static std::vector<std::shared_ptr<ModuleBase>> allModules_;
static std::map<ModuleBase::Handle, std::shared_ptr<ModuleBase>> windows_;
static std::atomic<bool> exitRequest_ = false;

static bool Init()
{
    if(!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    return true;
}

void AddWindowCallbacks(ModuleBase::Handle handle)
{
    glfwSetKeyCallback(handle, GLFWkeyfun(App::KeyCallback));
    glfwSetCharCallback(handle, GLFWcharfun(App::CharCallback));
    glfwSetMouseButtonCallback(handle, GLFWmousebuttonfun(App::MouseBtnCallback));
    glfwSetCursorPosCallback(handle, GLFWcursorposfun(App::CursorPosCallback));
    glfwSetScrollCallback(handle, GLFWscrollfun(App::ScrollCallback));
}

void App::AddModule(std::shared_ptr<ModuleBase> module)
{
    allModules_.push_back(module);
}

bool App::Run()
{
    if (!Init())
    {
        allModules_.clear();
        windows_.clear();
        return false;
    }

    bool isOk = true;
    for(auto& module : allModules_)
    {
        if (!module->Create())
        {
            isOk = false;
            break;
        }
        if (auto windowHandle = module->GetWindowHandle();
            nullptr != windowHandle)
        {
            windows_[windowHandle] = module;
            AddWindowCallbacks(windowHandle);
        }
    }
    if (isOk)
    {
        for(auto module : allModules_)
        {
            module->StartLoop();
        }
    }

    while(isOk && !exitRequest_)
    {
        glfwPollEvents();
        for(auto& [handle, window] : windows_)
        {
            if(glfwWindowShouldClose(handle) ||
               window->ShouldExit())
            {
                exitRequest_.store(true);
            }
        }
        if (exitRequest_)
        {
            for(auto module : allModules_)
            {
                module->ExitRequest();
            }
            for(auto module : allModules_)
            {
                module->WaitExit();
            }
            break;
        }
        std::this_thread::sleep_for(16ms);
    }

    std::this_thread::sleep_for(50ms);
    for(auto module : allModules_)
    {
        module->Destroy();
    }
    std::this_thread::sleep_for(50ms);
    allModules_.clear();
    windows_.clear();
    return true;
}
// key
void App::KeyCallback(ModuleBase::Handle w,int k,int s,int a,int m)
{
    if (auto it = windows_.find(w); it != windows_.end())
    {
        auto& window = it->second;
        window->PushEvent(Event{.type=EvType::Key, .key={k,s,a,m}});
    }
}
// char
void App::CharCallback(ModuleBase::Handle w,unsigned int cp)
{
    if (auto it = windows_.find(w); it != windows_.end())
    {
        auto& window = it->second;
        window->PushEvent(Event{.type=EvType::Char,.ch={cp}});
    }
}
// mouse btn
void App::MouseBtnCallback(ModuleBase::Handle w,int b,int a,int m)
{
    if (auto it = windows_.find(w); it != windows_.end())
    {
        auto& window = it->second;
        window->PushEvent(Event{.type=EvType::MouseBtn,.mouse={b,a,m}});
    }
}
// cursor
void App::CursorPosCallback(ModuleBase::Handle w,double x,double y)
{
    if (auto it = windows_.find(w); it != windows_.end())
    {
        auto& window = it->second;
        window->PushEvent(Event{.type=EvType::CursorPos,.cursor={x,y}});
    }
}
// scroll
void App::ScrollCallback(ModuleBase::Handle w,double xo,double yo)
{
    if (auto it = windows_.find(w); it != windows_.end())
    {
        auto& window = it->second;
        window->PushEvent(Event{.type=EvType::Scroll,.scroll={xo,yo}});
    }
}

bool ModuleBase::CreateWindow(int width, int height, const char *title)
{
    windowHandle_ = glfwCreateWindow(width,height,title,nullptr,nullptr);
    return windowHandle_ != nullptr;
}

void ModuleBase::CloseWindow()
{
    if (windowHandle_)
    {
        glfwDestroyWindow(windowHandle_);
    }
}

ModuleBase::Handle ModuleBase::GetWindowHandle() const
{
    return windowHandle_;
}

const std::vector<Event> &ModuleBase::GetEvents()
{
    std::lock_guard<SpinLock> l(lock_);
    eventQueus_[clientEventQueue_].clear();
    std::swap(clientEventQueue_, freeEventQueue_);
    return eventQueus_[clientEventQueue_];
}

bool ModuleBase::ShouldExit() const
{
    return shoudExit_.load();
}

void ModuleBase::StartLoop()
{
    mainThread_ = std::thread([this](){
        if (windowHandle_)
        {
            glfwMakeContextCurrent(windowHandle_);
            if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
            {
                fprintf(stderr, "Failed to initialize GLAD\n");
                return;
            }
        }
        Loop();
    });
}

void ModuleBase::WaitExit()
{
    if (mainThread_.joinable())
    {
        mainThread_.join();
    }
}

void ModuleBase::ExitRequest()
{
    shoudExit_.store(true);
}

void ModuleBase::PushEvent(Event ev)
{
    std::lock_guard<SpinLock> l(lock_);
    eventQueus_[freeEventQueue_].push_back(std::move(ev));
    if (eventQueus_[freeEventQueue_].size() > 128)
    {
        eventQueus_[freeEventQueue_]
                .erase(
                    eventQueus_[freeEventQueue_].begin(),
                    eventQueus_[freeEventQueue_].begin() +
                    eventQueus_[freeEventQueue_].size() / 2);
    }


}

}

