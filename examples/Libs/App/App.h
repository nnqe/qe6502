#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include <fmt/format.h>
#include <Memory.h>
#include <atomic>
#include <Clock.h>

namespace qe::Examples
{
using namespace std::chrono;

class IApp
{
public:
    virtual ~IApp();
    virtual void Init() = 0;
    virtual void Draw() = 0;
    virtual void ProcessOnMinimize() = 0;
    virtual void CustomRender() = 0;
    virtual void Deinit() = 0;
};

class Program
{
public:
    struct Context
    {
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_AudioDeviceID audioDeviceId;
        SDL_AudioSpec audioSpecs;
        std::atomic<bool> done;
        ImVec4 clearColor;
        bool isMinimized;
        float dpiScale = 1.0f;
        Ptr<IApp> app;
    };

    static Context& Ctx();
    static int Run( Ptr<IApp> app );

};

}
