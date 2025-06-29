#pragma once
#include <App.h>
#include "Context.h"
#include <qe_appleII.h>
#include <array>
namespace qe::Examples::AppleII
{

class Display : public ModuleBase
{
public:
    void SetContext(Context ctx);

    bool IsReadyForRawFrame() const;
    bool HasReadyRawFrame() const;
    void NewRawFrame(qeaii_frame_t* rawFrame);

    // ModuleBase interface
public:
    bool Create() override;
    void Loop() override;
    void Destroy() override;

private:
    void InitFrameResources();
    void UploadFrame();
    void DrawFrame();
    Context ctx_;
    std::atomic_flag frameConsumed_;
    qeaii_frame_t* rawFrame_ = nullptr;
    std::array<uint8_t, qeaii_width * qeaii_height * 3> rgbFrame_;
};

}

