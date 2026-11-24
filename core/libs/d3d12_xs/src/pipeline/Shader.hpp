#pragma once

#include "../device/Device.hpp"

namespace AnyBox::Graphics {
bool SupportsNativeComputeWaveSize(Device& device, std::uint32_t waveSize);
}
