#include "Display.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace qe::Examples::AppleII
{

static const char* VSrc()
{
    static const char* vsrc = R"(#version 330 core
    layout(location=0)in vec2 pos;
    void main(){gl_Position=vec4(pos,0.0,1.0);})";
    return vsrc;
}

static const char* FSrc()
{
    static const char* fsrc = R"(#version 330 core
    out vec4 color;
    void main(){color=vec4(1.0,0.4,0.2,1.0);})";
    return fsrc;
}

static GLuint MakeProgram()
{
    auto compile = [](GLenum type,const char* src){
        GLuint s=glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs=compile(GL_VERTEX_SHADER,VSrc());
    GLuint fs=compile(GL_FRAGMENT_SHADER,FSrc());
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void Display::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool Display::Create()
{
    return CreateWindow(800, 600, "AppleII Display");
}

void Display::Loop()
{
    GLuint vao,vbo;
    GLfloat verts[]={-0.6f,-0.5f, 0.6f,-0.5f, 0.0f,0.6f};
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(0);

    GLuint prog=MakeProgram();

    auto win = GetWindowHandle();

    while(!ShouldExit())
    {
        const auto& events = GetEvents();
        for (const auto& e : events)
        {
            if (e.type == EvType::Key)
            {
                if(e.key.key == GLFW_KEY_ESCAPE && e.key.action == GLFW_PRESS)
                {
                    ExitRequest();
                }
            }
        }

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

void Display::Destroy()
{

}


}
