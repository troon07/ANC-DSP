#pragma once
#include <cstddef>
#include <cstdint>
namespace anc {
struct DspConfig { uint32_t sampleRate=8000; size_t controllerTaps=64; float stepSize=0.03f; };
enum class RouteSupport { Simulation, WiredFixture, BluetoothDiagnostics, Unsupported };
const char* getVersionString() noexcept;
}
