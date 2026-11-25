#include "Shader.hpp"

bool AnyBox::Graphics::SupportsNativeComputeWaveSize(Device& device, std::uint32_t waveSize) {
    auto limits = device.GetWaveLimits();
    return limits[0] == waveSize && limits[1] == waveSize;
}
