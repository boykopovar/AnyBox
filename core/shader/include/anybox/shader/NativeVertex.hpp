#pragma once

#include "Native.hpp"
#include "VertexCompiler.hpp"
#include <anybox/graphics/container/NativeStage.hpp>
#include <Recompiler.hpp>

namespace AnyBox::Shader {
struct NativeVertexSystemRegisters {
    std::int32_t vertexOffset = -1;
    std::int32_t instanceOffset = -1;
};
ShaderRecompiler::IrProgram LowerNativeVertex(const NativeFrontend& frontend, const Graphics::Container::NativeStageState& state, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, NativeVertexSystemRegisters system = {});
StageCompilation CompileNativeVertex(std::span<const std::byte> program, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
