#pragma once

#include "Resources.hpp"
#include <array>
#include <string>

namespace AnyBox::Shader {
using ShaderDigest = std::array<std::byte, 32>;
ShaderDigest HashShader(std::span<const std::byte> data);
ShaderDigest HashShaderParts(std::span<const std::span<const std::byte>> parts);
std::string DigestName(const ShaderDigest& digest);

struct ArtifactDescriptor {
    std::uint32_t kind;
    std::vector<std::uint32_t> resources;
    bool operator==(const ArtifactDescriptor&) const = default;
};

struct ShaderArtifact {
    ShaderDigest sourceHash{};
    std::uint32_t profile = 1;
    std::uint32_t runtimeVersion = 14;
    std::uint32_t bdaVersion = 1;
    std::uint32_t waveSize = 32;
    std::uint32_t hostSubgroupSize = 32;
    std::uint32_t pushConstantOffset = 0;
    std::uint32_t pushConstantSize = 0;
    std::uint32_t pushDataStart = UINT32_MAX;
    std::uint32_t stencilReference = UINT32_MAX;
    std::uint32_t memoryOffset = 0;
    std::uint32_t memoryOffsetCount = 0;
    bool dispatchThreadLimit = false;
    std::uint32_t runtimeImageCount = 0;
    std::uint32_t runtimeStateDwords = 0;
    std::vector<std::uint32_t> userRegisters;
    std::vector<ArtifactDescriptor> descriptors;
    std::vector<BufferBinding> buffers;
    std::vector<std::uint32_t> code;
    bool operator==(const ShaderArtifact&) const = default;
};
void ValidateShaderArtifact(const ShaderArtifact& artifact);
}
