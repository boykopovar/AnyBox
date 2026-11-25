#pragma once

#include "NativeStage.hpp"

namespace AnyBox::Graphics::Container {
struct SerializedGraphicsShaderState {
    NativeStageState shader;
    std::uint32_t userRegisters;
    std::uint32_t waveSize;
};

SerializedGraphicsShaderState ReadSerializedGraphicsShaderState(std::span<const std::byte> state, NativeHardwareStage stage);
}
