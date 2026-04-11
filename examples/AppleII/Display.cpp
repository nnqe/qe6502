#include "Display.h"
#include <Stopwatch.h>
#include <qe_appleIIhelpers.h>

namespace qe::Examples::AppleII
{

void Display::RunModule(Context ctx)
{
    ctx_ = ctx;
    rgbTexture_ = SDL_CreateTexture(
        Program::Ctx().renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        280,   // pixel buffer width
        192    // pixel buffer height
        );

    if (!rgbTexture_)
    {
        fmt::println("Failed to create RGB texture: {}", SDL_GetError());
        exit(-1);
    }
    frameConsumed_.test_and_set();
}

void Display::DestroyModule()
{
    frameConsumed_.test_and_set();
    if (rgbTexture_)
    {
        SDL_DestroyTexture(rgbTexture_);
        rgbTexture_ = nullptr;
    }
}

void Display::Draw()
{
    if (HasReadyRawFrame())
    {
        UploadRgbBuffer();
        frameConsumed_.test_and_set();
    }
}

void Display::Render()
{
    int winW, winH;
    //SDL_GetRendererOutputSize(Program::Ctx().renderer, &winW, &winH);
    SDL_GetWindowSize(Program::Ctx().window, &winW, &winH);

    const int texW = 280, texH = 192;
    float scaleX = winW / float(texW);
    float scaleY = winH / float(texH);
    float scale  = (scaleX < scaleY ? scaleX : scaleY) * 0.98;

    SDL_Rect dst;
    dst.w = int(texW * scale);
    dst.h = int(texH * scale);
    dst.x = 8;//(winW - dst.w) / 2;
    dst.y = 8;//(winH - dst.h) / 2;

    SDL_RenderCopy(Program::Ctx().renderer, rgbTexture_, nullptr, &dst);
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

void Display::UploadRgbBuffer()
{
    // pitch = width * bytes_per_pixel
    qeaii_to_rgb(rawFrame_, rgbFrame_.data());

    const int pitch = 280 * 3;
    SDL_UpdateTexture(
        rgbTexture_,
        nullptr,
        rgbFrame_.data(),
        pitch
        );
}

void upload_rgb_buffer()
{

}

}
