#pragma once

#include "ShaderArtifact.hpp"

namespace AnyBox::Shader {
struct SerializedComputeSource {
    std::span<const std::byte> dispatchState;
    std::span<const std::byte> auxiliaryState;
    std::span<const std::byte> code;
    std::span<const std::byte> metadata;
};

ShaderDigest HashSerializedComputeSource(const SerializedComputeSource& source, const ShaderDigest& root, std::uint32_t subgroupSize);
ShaderDigest HashSerializedRootSignature(std::span<const std::byte> data);
}
