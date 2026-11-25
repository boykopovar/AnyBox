#pragma once

#include "GraphicsLayout.hpp"
#include <SpirvBackend/SpirvEmitter.hpp>
#include <optional>

namespace AnyBox::Shader {
struct GraphicsCompilation {
    GraphicsArtifact vertex;
    std::optional<GraphicsArtifact> pixel;
};
GraphicsCompilation CompileNativeGraphics(std::span<const std::byte> vertex, std::span<const std::byte> pixel, const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout, const ShaderRecompiler::SpirvTargetOptions& target);
}
