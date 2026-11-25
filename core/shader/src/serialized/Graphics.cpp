#include "Graphics.hpp"
#include <anybox/shader/NativeShaderArguments.hpp>
#include <anybox/shader/SerializedRootArguments.hpp>

AnyBox::Shader::SerializedGraphicsStageCompilation AnyBox::Shader::SerializedGraphics::Finish(ShaderRecompiler::IrProgram& program, StageCompilation compiled, const Graphics::Container::SerializedStageMetadata& metadata, const Graphics::Container::RootSignatureLayout& root, std::uint32_t visibility) {
    auto plan = ShaderRecompiler::ResourceMaterializer{}.ExtractPlan(program);
    auto roots = BuildSerializedStageRoots(metadata.resources, root, visibility);
    (void)NativeShaderArguments(plan, root, roots, compiled.bindings.layout.userDataRegisters, NativeRootBindingMode::SerializedSlots);
    return {std::move(compiled), std::move(plan), std::move(roots)};
}

AnyBox::Shader::SerializedGraphicsStageCompilation AnyBox::Shader::CompileSerializedVertex(const SerializedGraphicsStageSource& source, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::ShaderVertexInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, NativeVertexSystemRegisters system) {
    auto state = Graphics::Container::ReadSerializedGraphicsShaderState(source.state, Graphics::Container::NativeHardwareStage::Vertex);
    auto metadata = Graphics::Container::ReadSerializedStageMetadata(source.code, source.metadata);
    NativeFrontend frontend(source.code.first(static_cast<std::size_t>(metadata.codeWords) * 4), source.waveSize);
    auto program = LowerNativeVertex(frontend, state.shader, input, state.userRegisters, target, system);
    auto compiled = CompileVertex(program, input, target, {0, 0, 0, 128});
    return SerializedGraphics::Finish(program, std::move(compiled), metadata, root, 1);
}

AnyBox::Shader::SerializedGraphicsStageCompilation AnyBox::Shader::CompileSerializedPixel(const SerializedGraphicsStageSource& source, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::ShaderPixelInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target) {
    auto state = Graphics::Container::ReadSerializedGraphicsShaderState(source.state, Graphics::Container::NativeHardwareStage::Pixel);
    auto metadata = Graphics::Container::ReadSerializedStageMetadata(source.code, source.metadata);
    NativeFrontend frontend(source.code.first(static_cast<std::size_t>(metadata.codeWords) * 4), source.waveSize);
    auto program = LowerNativePixel(frontend, state.shader, input, state.userRegisters, target);
    auto compiled = CompilePixel(program, input, target, {0, 0, 0, 128});
    return SerializedGraphics::Finish(program, std::move(compiled), metadata, root, 5);
}
