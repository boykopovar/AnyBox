#pragma once

#include "Fspx.hpp"
#include "NativeBindings.hpp"

namespace AnyBox::Graphics::Container {
struct SerializedComputeMetadata {
    std::uint16_t control;
    std::array<std::byte, 16> shaderHash;
    std::vector<NativeResourceBinding> resources;
};

SerializedComputeMetadata ReadSerializedComputeMetadata(const FspxView& program);
SerializedComputeMetadata ReadSerializedComputeMetadata(std::span<const std::byte> code, std::span<const std::byte> records);
}
