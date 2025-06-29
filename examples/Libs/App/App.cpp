#include "App.h"

namespace qe::Examples
{

IApp::~IApp() = default;

static bool InitSdl(Program::Context& ctx)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        fmt::println("Error: {}", SDL_GetError());
        return false;
    }

// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    ctx.window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);
    if (ctx.window == nullptr)
    {
        fmt::println("Error: SDL_CreateWindow(): {}", SDL_GetError());
        return false;
    }
    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (ctx.renderer == nullptr)
    {
        fmt::println("Error creating SDL_Renderer!");
        return false;
    }
    return true;
}

void CloseSdl(Program::Context& ctx)
{
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}


static bool InitGui(Program::Context& ctx)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(ctx.window, ctx.renderer);
    ImGui_ImplSDLRenderer2_Init(ctx.renderer);

    // Our state
    ctx.clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ctx.done = false;
    ctx.isMinimized = false;
    return true;
}


static void InitDpi(Program::Context& ctx)
{
    int displayIndex = SDL_GetWindowDisplayIndex(ctx.window);
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(displayIndex, &dm) != 0) {
        SDL_Log("SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
        dm.w = 1920; dm.h = 1080; // fallback
    }

    float scaleX = dm.w / 1280.0f;
    float scaleY = dm.h / 720;

    ctx.dpiScale = std::min(scaleX, scaleY);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = ctx.dpiScale;
    ImGui::GetStyle().ScaleAllSizes(ctx.dpiScale);
}


static void ProcessEvents(Program::Context& ctx)
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            ctx.done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(ctx.window))
            ctx.done = true;
    }
    if (SDL_GetWindowFlags(ctx.window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        ctx.isMinimized = true;
    }
    else
    {
        ctx.isMinimized = false;
    }
}

static void CloseGui(Program::Context& ctx)
{
    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

static void StartGuiFrame(Program::Context& ctx)
{
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

static void PrepareRendering(Program::Context& ctx)
{
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_RenderSetScale(ctx.renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(ctx.renderer, (Uint8)(ctx.clearColor.x * 255), (Uint8)(ctx.clearColor.y * 255), (Uint8)(ctx.clearColor.z * 255), (Uint8)(ctx.clearColor.w * 255));
    SDL_RenderClear(ctx.renderer);
}

static void Present(Program::Context& ctx)
{
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx.renderer);
    SDL_RenderPresent(ctx.renderer);
}

Program::Context& Program::Ctx()
{
    static Context s_ctx;
    return s_ctx;
}

int Program::Run( Ptr<IApp> appPtr )
{
    auto& ctx = Ctx();
    ctx.done = false;
    ctx.app = std::move(appPtr);
    auto& app = *ctx.app;

    if (!InitSdl( ctx ))
    {
        return -1;
    }
    if (!InitGui( ctx ))
    {
        CloseSdl( ctx );
        return -1;
    }
    InitDpi( ctx );
    app.Init();

    while(!ctx.done)
    {
        ProcessEvents( ctx );
        if (ctx.isMinimized)
        {
            app.ProcessOnMinimize();
            SDL_Delay(10);
            continue;
        }
        StartGuiFrame( ctx );
        app.Draw();
        PrepareRendering( ctx );
        app.CustomRender();
        Present( ctx );
    }

    ctx.done = true;
    app.Deinit();
    CloseGui( ctx );
    CloseSdl( ctx );
    return 0;
}

}
