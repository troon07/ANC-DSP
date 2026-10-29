#pragma once

#include "anc/audio_frame.h"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace anc {

struct AudioDeviceInfo {
    std::string id;
    std::string name;
    bool is_default = false;
    uint32_t default_sample_rate = 48000;
    uint32_t channels = 2;
};

using AudioCallback = std::function<void(const AudioFrame& in, AudioFrame& out)>;

/**
 * @brief Platform-independent Audio Backend Abstraction.
 */
class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    virtual std::vector<AudioDeviceInfo> enumerateCaptureDevices() = 0;
    virtual std::vector<AudioDeviceInfo> enumerateRenderDevices() = 0;

    virtual bool openStream(const std::string& capture_id,
                            const std::string& render_id,
                            uint32_t sample_rate,
                            size_t block_size) = 0;

    virtual void closeStream() = 0;
    virtual bool isRunning() const = 0;

    virtual void setCallback(AudioCallback cb) = 0;

    virtual size_t getUnderrunCount() const = 0;
    virtual float getEstimatedRoundtripLatencyMs() const = 0;
    virtual uint32_t getActualSampleRate() const = 0;
    virtual size_t getActualBlockSize() const = 0;
};

} // namespace anc
