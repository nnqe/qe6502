// main.cpp  – C++14, GLFW + GLEW, два прозореца / два контекста / три нишки
//  ▪ windowGui  – ImGui бутон
//  ▪ windowTri  – обикновен OpenGL триъгълник

#include <atomic>
#include <thread>
#include <vector>
#include <mutex>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

//------------------------------------------------------------
// event relay (минимален)
enum class EvType { Key, Char, MouseBtn, CursorPos, Scroll };
struct Event {
    EvType type;
    union {
        struct { int key, scancode, action, mods; } key;
        struct { unsigned int codepoint; } ch;
        struct { int button, action, mods; } mouse;
        struct { double x, y; } cursor;
        struct { double xoff, yoff; } scroll;
    };
};

// две опашки – по една за всеки рендер-тред
std::vector<Event> queueGui,   queueTri;
std::mutex         mtxGui,     mtxTri;

std::atomic_bool quit{false};

//------------------------------------------------------------
// помощна функция: към коя опашка принадлежи прозорецът?
inline std::pair<std::vector<Event>*, std::mutex*>
selectQueue(GLFWwindow* w, GLFWwindow* guiW, GLFWwindow* triW)
{
    return (w == guiW) ? std::make_pair(&queueGui, &mtxGui)
                       : std::make_pair(&queueTri, &mtxTri);
}

//------------------------------------------------------------
// GLFW callbacks (регистрират се само в main thread)
#define FORWARD(cb, code)                                                          \
[](GLFWwindow* win, code) {                                                    \
        auto [q, m] = selectQueue(win, windowGui, windowTri);                      \
        std::lock_guard<std::mutex> lk(*m);                                        \
        q->push_back({EvType::cb, {.cb = {__VA_ARGS__}}});                         \
}

GLFWwindow* windowGui = nullptr;
GLFWwindow* windowTri = nullptr;

// key
void keyCallback(GLFWwindow* w,int k,int s,int a,int m) {
    auto [q,mtx]=selectQueue(w,windowGui,windowTri);
    std::lock_guard<std::mutex> lk(*mtx);
    q->push_back(Event{.type=EvType::Key, .key={k,s,a,m}});
    if(k==GLFW_KEY_ESCAPE && a==GLFW_PRESS) quit=true;
}
// char
void charCallback(GLFWwindow* w,unsigned int cp){
    auto[q,mtx]=selectQueue(w,windowGui,windowTri);
    std::lock_guard<std::mutex> lk(*mtx);
    q->push_back(Event{.type=EvType::Char,.ch={cp}});
}
// mouse btn
void mouseBtnCallback(GLFWwindow* w,int b,int a,int m){
    auto[q,mtx]=selectQueue(w,windowGui,windowTri);
    std::lock_guard<std::mutex> lk(*mtx);
    q->push_back(Event{.type=EvType::MouseBtn,.mouse={b,a,m}});
}
// cursor
void cursorPosCallback(GLFWwindow* w,double x,double y){
    auto[q,mtx]=selectQueue(w,windowGui,windowTri);
    std::lock_guard<std::mutex> lk(*mtx);
    q->push_back(Event{.type=EvType::CursorPos,.cursor={x,y}});
}
// scroll
void scrollCallback(GLFWwindow* w,double xo,double yo){
    auto[q,mtx]=selectQueue(w,windowGui,windowTri);
    std::lock_guard<std::mutex> lk(*mtx);
    q->push_back(Event{.type=EvType::Scroll,.scroll={xo,yo}});
}

//------------------------------------------------------------
// минимални шейдъри за триъгълника
const char* vsrc = R"(#version 330 core
layout(location=0)in vec2 pos;
void main(){gl_Position=vec4(pos,0.0,1.0);})";

const char* fsrc = R"(#version 330 core
out vec4 color;
void main(){color=vec4(1.0,0.4,0.2,1.0);})";

GLuint makeProgram()
{
    auto compile = [](GLenum type,const char* src){
        GLuint s=glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs=compile(GL_VERTEX_SHADER,vsrc);
    GLuint fs=compile(GL_FRAGMENT_SHADER,fsrc);
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

//------------------------------------------------------------

void guiThreadFunc(GLFWwindow* win)
{
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE;
    glewInit();

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(win,false);
    ImGui_ImplOpenGL3_Init("#version 330");

    while(!quit){
        // дренираме опашката
        std::vector<Event> local;
        { std::lock_guard<std::mutex> lk(mtxGui); local.swap(queueGui); }

        for(auto& ev:local){
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

        ImGui::Begin("Demo");
        if(ImGui::Button("Close")) quit=true;
        ImGui::End();

        ImGui::Render();
        glViewport(0,0,800,600);
        glClearColor(0.1f,0.1f,0.1f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void triThreadFunc(GLFWwindow* win)
{
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE;
    glewInit();

    // VAO + VBO
    GLuint vao,vbo;
    GLfloat verts[]={-0.6f,-0.5f, 0.6f,-0.5f, 0.0f,0.6f};
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);

    GLuint prog=makeProgram();

    while(!quit){
        // дренираме (в случая ни трябват само close/ESC, вече обработени)
        { std::lock_guard<std::mutex> lk(mtxTri); queueTri.clear(); }

        glViewport(0,0,800,600);
        glClearColor(0.f,0.f,0.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES,0,3);
        glfwSwapBuffers(win);
    }

    glDeleteProgram(prog);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
}

//------------------------------------------------------------

int main()
{
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    windowGui = glfwCreateWindow(800,600,"ImGui Window",nullptr,nullptr);
    windowTri = glfwCreateWindow(800,600,"Triangle Window",nullptr,nullptr);
    if(!windowGui||!windowTri){ glfwTerminate(); return -1; }

    // глобални callbacks
    glfwSetKeyCallback      (windowGui,keyCallback);
    glfwSetCharCallback     (windowGui,charCallback);
    glfwSetMouseButtonCallback(windowGui,mouseBtnCallback);
    glfwSetCursorPosCallback(windowGui,cursorPosCallback);
    glfwSetScrollCallback   (windowGui,scrollCallback);

    glfwSetKeyCallback      (windowTri,keyCallback);
    glfwSetCharCallback     (windowTri,charCallback);
    glfwSetMouseButtonCallback(windowTri,mouseBtnCallback);
    glfwSetCursorPosCallback(windowTri,cursorPosCallback);
    glfwSetScrollCallback   (windowTri,scrollCallback);

    std::thread tGui(guiThreadFunc,windowGui);
    std::thread tTri(triThreadFunc,windowTri);

    while(!quit){
        glfwPollEvents();
        if(glfwWindowShouldClose(windowGui)||glfwWindowShouldClose(windowTri))
            quit=true;
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    tGui.join();
    tTri.join();
    glfwDestroyWindow(windowGui);
    glfwDestroyWindow(windowTri);
    glfwTerminate();
    return 0;
}
