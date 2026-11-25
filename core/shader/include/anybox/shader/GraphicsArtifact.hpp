#pragma once

#include "ShaderArtifact.hpp"

namespace AnyBox::Shader {
enum class GraphicsArtifactStage : std::uint32_t { Vertex = 0, Pixel = 4 };

struct GraphicsArtifactKey {
    GraphicsArtifactStage stage;
    ShaderDigest sourceHash;
    ShaderDigest layoutHash;
    bool operator==(const GraphicsArtifactKey&) const = default;
};

struct GraphicsArtifact : ShaderArtifact {
    GraphicsArtifactStage stage = GraphicsArtifactStage::Vertex;
    ShaderDigest layoutHash{};
    GraphicsArtifactKey GetKey() const noexcept { return {stage, sourceHash, layoutHash}; }
    bool operator==(const GraphicsArtifact&) const = default;
};

ShaderDigest HashGraphicsArtifactKey(const GraphicsArtifactKey& key);
void ValidateGraphicsArtifact(const GraphicsArtifact& artifact);
std::vector<std::byte> EncodeGraphicsArtifact(const GraphicsArtifact& artifact);
GraphicsArtifact DecodeGraphicsArtifact(std::span<const std::byte> data, const GraphicsArtifactKey& expected);
}
