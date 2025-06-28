#include "Audio.h"
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
namespace qe::Examples::Audio
{

static SourceCbk s_cbk;
static void Callback(void*, void* output, const void*, uint32_t frameCount)
{
    if (s_cbk)
    {
        s_cbk(output, frameCount);
    }
}

static ma_device s_device;
bool Init(uint32_t format, uint32_t channels, uint32_t sampleRate, uint32_t periodSizeInSamples, SourceCbk cbk, void *userObj)
{
    s_cbk = std::move(cbk);
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format(format);
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = ma_device_data_proc(Callback);
    config.periodSizeInFrames = periodSizeInSamples;
    config.pUserData         = userObj;

    if (ma_device_init(NULL, &config, &s_device) != MA_SUCCESS)
    {
        return false;
    }

    ma_device_start(&s_device);
    return true;
}

void Deinit()
{
    ma_device_uninit(&s_device);
}


}
