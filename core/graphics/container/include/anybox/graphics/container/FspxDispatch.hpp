#pragma once

#include "NativeCompute.hpp"

namespace AnyBox::Graphics::Container {
struct SerializedComputeState {
    NativeComputeState shader;
    std::uint32_t waveSize;
};

SerializedComputeState ReadSerializedComputeState(std::span<const std::byte> data);
}
