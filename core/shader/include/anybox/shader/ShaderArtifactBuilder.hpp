#pragma once

#include "Compilation.hpp"
#include "GraphicsArtifact.hpp"

namespace AnyBox::Shader {
ShaderArtifact MakeShaderArtifact(StageCompilation result, const ShaderDigest& source, std::span<const BufferBinding> buffers, std::uint32_t waveSize, std::uint32_t hostSubgroupSize);
GraphicsArtifact MakeGraphicsArtifact(StageCompilation result, const GraphicsArtifactKey& key, std::uint32_t waveSize, std::uint32_t hostSubgroupSize, std::span<const BufferBinding> buffers = {});
}
