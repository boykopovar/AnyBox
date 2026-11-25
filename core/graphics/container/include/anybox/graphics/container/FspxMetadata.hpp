#pragma once

#include "Fspx.hpp"
#include "NativeBindings.hpp"

namespace AnyBox::Graphics::Container {
struct SerializedComputeMetadata {
    std::array<std::byte, 16> shaderHash;
    std::vector<NativeResourceBinding> resources;
};

SerializedComputeMetadata ReadSerializedComputeMetadata(const FspxView& program);
}
