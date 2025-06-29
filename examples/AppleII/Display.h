#pragma once
#include "Context.h"
#include <App.h>
#include <qe_appleII.h>
#include <SDL.h>
#include <array>

namespace qe::Examples::AppleII
{

class Display
{
public:
    void RunModule(Context ctx);
    void DestroyModule();
    void Draw();
    void Render();

    bool IsReadyForRawFrame() const;
    bool HasReadyRawFrame() const;
    void NewRawFrame(qeaii_frame_t* rawFrame);
    void ClearDisplay();

private:

    void UploadRgbBuffer();

    Context ctx_;
    std::atomic_flag frameConsumed_;
    std::atomic<bool> clearDisplay_ = false;
    qeaii_frame_t* rawFrame_ = nullptr;
    std::array<uint8_t, qeaii_width * qeaii_height * 3> rgbFrame_;
    SDL_Texture*  rgbTexture_;
};

}

