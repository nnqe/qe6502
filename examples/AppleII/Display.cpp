#include "Display.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <Stopwatch.h>
#include <qe_appleIIhelpers.h>

namespace qe::Examples::AppleII
{


static GLuint frameTex = 0;
static GLuint quadVAO = 0, quadVBO = 0;
static GLuint frameProg = 0;

static const char* TexVSrc()
{
    return R"(#version 330 core
    layout(location = 0) in vec2 pos;
    layout(location = 1) in vec2 uv;
    out vec2 TexCoord;
    void main() {
        TexCoord = uv;
        gl_Position = vec4(pos, 0.0, 1.0);
    })";
}

static const char* TexFSrc()
{
    return R"(#version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D frameTex;
    void main() {
        FragColor = texture(frameTex, TexCoord);
    })";
}

static GLuint MakeTexProgram()
{
    auto compile = [](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, TexVSrc());
    GLuint fs = compile(GL_FRAGMENT_SHADER, TexFSrc());
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static auto FramebufferSizeCallback = [](GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
};

void Display::SetContext(Context ctx)
{
    ctx_ = ctx;
}

bool Display::IsReadyForRawFrame() const
{
    return frameConsumed_.test();
}

bool Display::HasReadyRawFrame() const
{
    return !frameConsumed_.test();
}

void Display::NewRawFrame(qeaii_frame_t *rawFrame)
{
    if (HasReadyRawFrame())
    {
        fmt::println("New frame cannot be set. Possible race-condititon. please fix it !");
        exit(-11);
    }
    rawFrame_ = rawFrame;
    frameConsumed_.clear();
}

void Display::ClearDisplay()
{
    clearDisplay_ = true;
}

bool Display::Create()
{
    frameConsumed_.test_and_set();
    return CreateWindow(800, 600, "AppleII Display");
}

void Display::Loop()
{
    glfwSetFramebufferSizeCallback(GetWindowHandle(), FramebufferSizeCallback);
    InitFrameResources();

    auto win = GetWindowHandle();
    glViewport(0, 0, 800, 600);

    Stopwatch sw;
    auto start = sw.Measure();
    int frames = 0;
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
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (clearDisplay_)
        {
            clearDisplay_ = false;
            memset(rgbFrame_.data(), 0, rgbFrame_.size());
            UploadFrame();
        }
        if (HasReadyRawFrame())
        {
            qeaii_frame_to_rgb(rawFrame_, rgbFrame_.data());
            frameConsumed_.test_and_set();
            UploadFrame();
        }
        DrawFrame();

        glfwSwapBuffers(win);
        frames++;
        if (0 == (frames % 100))
        {
            double sec = (sw.Measure() - start).count() / 1'000'000'000.0;
            double fps = double(frames) / sec;
            fmt::println("FPS: {}", fps);
        }
    }

    glDeleteProgram(frameProg);
    glDeleteBuffers(1,&quadVBO);
    glDeleteVertexArrays(1,&quadVAO);
}

void Display::Destroy()
{

}

void Display::InitFrameResources()
{
    frameProg = MakeTexProgram();

    glGenTextures(1, &frameTex);
    glBindTexture(GL_TEXTURE_2D, frameTex);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB,
        qeaii_width, qeaii_height, 0,
        GL_RGB, GL_UNSIGNED_BYTE,
        rgbFrame_.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLfloat quadVerts[] = {
        //  pos        uv
        -1.f, -1.f,   0.f, 1.f,   // bottom-left  -> tex (0,1)
         1.f, -1.f,   1.f, 1.f,   // bottom-right -> tex (1,1)
         1.f,  1.f,   1.f, 0.f,   // top-right    -> tex (1,0)

        -1.f, -1.f,   0.f, 1.f,   // bottom-left
         1.f,  1.f,   1.f, 0.f,   // top-right
        -1.f,  1.f,   0.f, 0.f    // top-left     -> tex (0,0)
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    // pos attrib
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // uv attrib
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Display::UploadFrame()
{
    glBindTexture(GL_TEXTURE_2D, frameTex);
    glTexSubImage2D(
        GL_TEXTURE_2D, 0,
        0, 0,          // xoffset, yoffset
        qeaii_width, qeaii_height,      // width, height
        GL_RGB, GL_UNSIGNED_BYTE,
        rgbFrame_.data()
    );
}

void Display::DrawFrame()
{
    glUseProgram(frameProg);
    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(frameProg, "frameTex"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

}
