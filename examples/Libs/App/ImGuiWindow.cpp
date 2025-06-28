#include "ImGuiWindow.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace qe::Examples
{
bool ImGuiWindow::Create()
{
    std::string title;
    if (false == CreateModule(width_, height_, title))
    {
        return false;
    }
    if (false == CreateWindow(width_, height_, title.c_str()))
    {
        return false;
    }
    return true;
}

void ImGuiWindow::Loop()
{
    auto win = GetWindowHandle();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(win ,false);
    ImGui_ImplOpenGL3_Init("#version 330");

    while(!ShouldExit())
    {
        const auto& events = GetEvents();

        for(auto& ev:events)
        {
            switch(ev.type){
            case EvType::Key:       ImGui_ImplGlfw_KeyCallback      (win,ev.key.key,ev.key.scancode,ev.key.action,ev.key.mods); break;
            case EvType::Char:      ImGui_ImplGlfw_CharCallback     (win,ev.ch.codepoint); break;
            case EvType::MouseBtn:  ImGui_ImplGlfw_MouseButtonCallback(win,ev.mouse.button,ev.mouse.action,ev.mouse.mods); break;
            case EvType::CursorPos: ImGui_ImplGlfw_CursorPosCallback(win,ev.cursor.x,ev.cursor.y); break;
            case EvType::Scroll:    ImGui_ImplGlfw_ScrollCallback   (win,ev.scroll.xoff,ev.scroll.yoff); break;
            default: break;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            RenderModule();
        }
        ImGui::Render();
        glViewport(0,0,width_,height_);
        glClearColor(0.1f,0.1f,0.1f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWindow::Destroy()
{
    CloseModule();
    CloseWindow();
}

}
