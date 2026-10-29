#pragma once
#include <cstddef>
#include <cstdint>
namespace anc {
// Interleaved input: channel 0 reference, channel 1 measured residual error.
struct AudioFrame { float* data=nullptr; size_t frames=0; uint32_t channels=2; uint32_t sampleRate=48000; };
}
