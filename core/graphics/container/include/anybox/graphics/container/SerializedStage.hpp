#pragma once

#include "NativeBindings.hpp"
#include <array>

namespace AnyBox::Graphics::Container {
struct SerializedStageMetadata {
    std::uint32_t codeWords;
    std::uint16_t control;
    std::array<std::byte, 16> shaderHash;
    std::vector<NativeResourceBinding> resources;
    std::vector<std::array<std::byte, 20>> declarations;
};

SerializedStageMetadata ReadSerializedStageMetadata(std::span<const std::byte> code, std::span<const std::byte> records);
}
