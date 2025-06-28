#pragma once

#include <cstdint>
#include <functional>

namespace qe::Examples::Audio
{

static constexpr uint32_t Format_unknown = 0;     /* Mainly used for indicating an error, but also used as the default for the output format for decoders. */
static constexpr uint32_t Format_u8      = 1;
static constexpr uint32_t Format_s16     = 2;     /* Seems to be the most widely supported format. */
static constexpr uint32_t Format_s24     = 3;     /* Tightly packed. 3 bytes per sample. */
static constexpr uint32_t Format_s32     = 4;
static constexpr uint32_t Format_f32     = 5;

using SourceCbk = std::function<void(void* output, uint32_t frameCount)>;

bool Init(uint32_t format,
          uint32_t channels,
          uint32_t sampleRate,
          uint32_t periodSizeInSamples,
          SourceCbk,
          void* userObj);

void Deinit();

}
