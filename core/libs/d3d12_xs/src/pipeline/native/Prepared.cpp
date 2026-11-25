#include "Prepared.hpp"
#include "../Cache.hpp"
#include <anybox/graphics/container/FspxDispatch.hpp>

AnyBox::Graphics::PreparedNativeCompute AnyBox::Graphics::LoadPreparedNativeCompute(const Shader::SerializedComputeSource& source, const Shader::ShaderDigest& root, std::uint32_t subgroupSize) {
    auto key = Shader::HashSerializedComputeSource(source, root, subgroupSize);
    auto name = Shader::DigestName(key);
    auto bytes = ReadPreparedArtifact(name + ".abxc", "Prepared serialized compute shader");
    auto shader = Shader::DecodeComputeArtifact(bytes, key);
    auto state = Container::ReadSerializedComputeState(source.dispatchState);
    if (shader.waveSize != state.waveSize || shader.hostSubgroupSize != subgroupSize || shader.threads != state.shader.threads || shader.sharedMemoryBytes != state.shader.declaredSharedBytes) throw std::invalid_argument("Prepared serialized compute shader state differs from its source");
    auto plan = ReadPreparedArtifact(name + ".abxr", "Prepared serialized compute resources");
    auto resources = Shader::DecodeResourcePlan(plan, Shader::HashShader(std::span(bytes).first(80)), ShaderRecompiler::IrShaderStage::Compute);
    return {std::move(shader), std::move(resources)};
}
