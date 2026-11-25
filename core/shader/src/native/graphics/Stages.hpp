#pragma once

#include <anybox/shader/GraphicsCompiler.hpp>
#include <anybox/shader/NativeArguments.hpp>
#include <anybox/shader/NativeLinkage.hpp>
#include <anybox/shader/ShaderArtifactBuilder.hpp>
#include <anybox/graphics/container/Native.hpp>
#include <Recompiler.hpp>

namespace AnyBox::Shader::NativeGraphics {
std::uint32_t UserRegisterCount(const Graphics::Container::NativeBindings& bindings);
GraphicsArtifact CompileVertex(std::span<const std::byte> program, const Graphics::Container::RootSignatureLayout& root, const GraphicsArtifactKey& key, const ShaderRecompiler::SpirvTargetOptions& target);
GraphicsArtifact CompilePixel(std::span<const std::byte> program, const Graphics::Container::NativeVertexOutputs& vertex, const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout, const GraphicsArtifactKey& key, const ShaderRecompiler::SpirvTargetOptions& target);
}
