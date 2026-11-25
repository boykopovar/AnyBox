#pragma once

#include "ShaderArtifact.hpp"

namespace AnyBox::Shader {
inline constexpr std::uint32_t NativeComputeInvocations = 16;
inline constexpr std::uint32_t SpannedComputeWave = 9;
struct ComputeArtifact : ShaderArtifact {
    std::array<std::uint32_t, 3> threads{};
    std::uint32_t sharedMemoryBytes = 0;
    std::uint32_t executionFlags = 0;
    std::uint32_t splitStateLanes = 0;
    std::uint32_t scratchPoolSlotDwords = 0;
    bool operator==(const ComputeArtifact&) const = default;
};

std::vector<std::byte> EncodeComputeArtifact(const ComputeArtifact& artifact);
void ValidateComputeArtifact(const ComputeArtifact& artifact);
ComputeArtifact DecodeComputeArtifact(std::span<const std::byte> data, const ShaderDigest& expectedSource);
}
