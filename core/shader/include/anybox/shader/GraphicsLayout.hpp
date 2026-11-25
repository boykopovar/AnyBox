#pragma once

#include "GraphicsArtifact.hpp"
#include <anybox/graphics/container/RootSignature.hpp>

namespace AnyBox::Shader {
struct GraphicsShaderLayout {
    ShaderDigest vertexSource{};
    ShaderDigest pixelSource{};
    std::uint32_t targetProfile = 1;
    std::uint32_t hostSubgroupSize = 32;
    std::array<std::uint32_t, 8> renderTargetFormats{};
    std::uint32_t renderTargetCount = 0;
    std::uint32_t depthStencilFormat = 0;
    std::uint32_t sampleCount = 1;
    std::uint32_t sampleQuality = 0;
};

ShaderDigest HashGraphicsLayout(const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout);
}
