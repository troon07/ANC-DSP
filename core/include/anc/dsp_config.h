#pragma once
#include <cstddef>
#include <cstdint>
namespace anc {
struct DspConfig {
    uint32_t sampleRate=8000;size_t controllerTaps=64;float stepSize=0.03f;
    float leakage=1e-6f,epsilon=1e-8f,maximumCoefficientNorm=20,minimumReferencePower=1e-10f;
    bool conditionReference=false;
    float dcCutoff=10,referenceLowpass=1500;
    float baselineErrorRms=0,maximumErrorGrowthDb=6;
};
enum class RouteSupport { Simulation, WiredFixture, BluetoothDiagnostics, Unsupported };
const char* getVersionString() noexcept;
}
