#include "Stages.hpp"
#include <anybox/shader/NativeVertex.hpp>

AnyBox::Shader::GraphicsArtifact AnyBox::Shader::NativeGraphics::CompileVertex(std::span<const std::byte> program, const Graphics::Container::RootSignatureLayout& root, const GraphicsArtifactKey& key, const ShaderRecompiler::SpirvTargetOptions& target) {
    auto bindings = Graphics::Container::ReadNativeBindings(program);
    auto buffers = ReadNativeDirectBufferBindings(bindings, root, 1);
    RootArgumentPlan plan(buffers, root, 0, 1);
    ShaderRecompiler::ShaderVertexInputInfo input;
    input.hostSubgroupSize = target.subgroupSize;
    NativeFrontend frontend(program);
    auto translated = LowerNativeVertex(frontend, Graphics::Container::ReadNativeStageState(program), input, UserRegisterCount(bindings), target);
    BindNativeGraphicsRootArguments(translated, bindings.roots, plan);
    auto compiled = AnyBox::Shader::CompileVertex(translated, input, target, {0, 0, 0, 128});
    return MakeGraphicsArtifact(std::move(compiled), key, frontend.GetWaveSize(), target.subgroupSize, buffers);
}
