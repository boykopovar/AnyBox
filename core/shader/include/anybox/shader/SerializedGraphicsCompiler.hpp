#pragma once

#include "NativeVertex.hpp"
#include "NativePixel.hpp"
#include <Optimization/ResourceMaterializer.hpp>
#include <anybox/graphics/container/RootSignature.hpp>
#include <anybox/graphics/container/NativeBindings.hpp>

namespace AnyBox::Shader {
struct SerializedGraphicsStageSource {
    std::span<const std::byte> state;
    std::span<const std::byte> code;
    std::span<const std::byte> metadata;
    std::uint32_t waveSize = 0;
};

struct SerializedGraphicsStageCompilation {
    StageCompilation shader;
    ShaderRecompiler::IrResourcePlan resources;
    std::vector<Graphics::Container::NativeRootBinding> roots;
};

SerializedGraphicsStageCompilation CompileSerializedVertex(const SerializedGraphicsStageSource& source, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::ShaderVertexInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, NativeVertexSystemRegisters system);
SerializedGraphicsStageCompilation CompileSerializedPixel(const SerializedGraphicsStageSource& source, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::ShaderPixelInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target);
ShaderRecompiler::ShaderPixelInputInfo ReadSerializedPixelInput(std::span<const std::byte> state, std::span<const std::byte> auxiliary);
SerializedGraphicsStageCompilation CompileSerializedPixel(const SerializedGraphicsStageSource& source, std::span<const std::byte> auxiliary, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::SpirvTargetOptions& target);
}
