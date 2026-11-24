#pragma once

#include "Resources.hpp"
#include <array>
#include <string>

namespace AnyBox::Shader {
inline constexpr std::uint32_t PreparedComputeWaveSize = 32;

using ShaderDigest = std::array<std::byte, 32>;
ShaderDigest HashShader(std::span<const std::byte> data);
std::string DigestName(const ShaderDigest& digest);

struct ArtifactDescriptor {
    std::uint32_t kind;
    std::vector<std::uint32_t> resources;
    bool operator==(const ArtifactDescriptor&) const = default;
};

struct ComputeArtifact {
    ShaderDigest sourceHash{};
    std::uint32_t profile = 1;
    std::uint32_t runtimeVersion = 14;
    std::uint32_t bdaVersion = 1;
    std::array<std::uint32_t, 3> threads{};
    std::uint32_t waveSize = PreparedComputeWaveSize;
    std::uint32_t sharedMemoryBytes = 0;
    std::uint32_t pushConstantOffset = 0;
    std::uint32_t pushConstantSize = 0;
    std::uint32_t pushDataStart = UINT32_MAX;
    std::uint32_t stencilReference = UINT32_MAX;
    std::uint32_t memoryOffset = 0;
    std::uint32_t memoryOffsetCount = 0;
    bool dispatchThreadLimit = false;
    std::uint32_t runtimeImageCount = 0;
    std::uint32_t runtimeStateDwords = 0;
    std::uint32_t executionFlags = 0;
    std::uint32_t splitStateLanes = 0;
    std::uint32_t scratchPoolSlotDwords = 0;
    std::vector<std::uint32_t> userRegisters;
    std::vector<ArtifactDescriptor> descriptors;
    std::vector<BufferBinding> buffers;
    std::vector<std::uint32_t> code;
    bool operator==(const ComputeArtifact&) const = default;
};

std::vector<std::byte> EncodeComputeArtifact(const ComputeArtifact& artifact);
void ValidateComputeArtifact(const ComputeArtifact& artifact);
ComputeArtifact DecodeComputeArtifact(std::span<const std::byte> data, const ShaderDigest& expectedSource);
}
