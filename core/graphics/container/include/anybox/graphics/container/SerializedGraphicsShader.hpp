#pragma once

#include "NativeStage.hpp"

namespace AnyBox::Graphics::Container {
struct SerializedGraphicsShaderState {
    NativeStageState shader;
    std::uint32_t userRegisters;
};

SerializedGraphicsShaderState ReadSerializedGraphicsShaderState(std::span<const std::byte> state, NativeHardwareStage stage);
}
