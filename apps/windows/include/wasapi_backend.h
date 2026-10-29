#pragma once
#include "audio_backend.h"
#include <memory>
namespace anc {
class WasapiBackend : public AudioBackend {
    struct Impl; std::unique_ptr<Impl> p_;
public:
    WasapiBackend(); ~WasapiBackend() override;
    std::vector<AudioDeviceInfo> enumerateCaptureDevices() override;
    std::vector<AudioDeviceInfo> enumerateRenderDevices() override;
    bool openStream(const std::string&,const std::string&,uint32_t,size_t) override;
    void closeStream() override;
    bool isRunning() const override;
    void setCallback(AudioCallback cb) override;
    void setTonalMode(bool enabled);
    size_t getUnderrunCount() const override;
    float getEstimatedRoundtripLatencyMs() const override;
    uint32_t getActualSampleRate() const override;
    size_t getActualBlockSize() const override;
};
}
