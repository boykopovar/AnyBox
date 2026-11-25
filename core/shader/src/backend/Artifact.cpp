#include <anybox/shader/ShaderArtifactBuilder.hpp>
#include <RuntimeAbi.hpp>
#include <utility>

AnyBox::Shader::ShaderArtifact AnyBox::Shader::MakeShaderArtifact(StageCompilation result, const ShaderDigest& source, std::span<const BufferBinding> buffers, std::uint32_t waveSize, std::uint32_t hostSubgroupSize) {
    static_assert(ShaderRecompiler::RuntimeAbi::Version == ShaderArtifact{}.runtimeVersion);
    ShaderArtifact artifact;
    artifact.sourceHash = source;
    artifact.waveSize = waveSize;
    artifact.hostSubgroupSize = hostSubgroupSize;
    artifact.pushConstantOffset = result.bindings.pushConstantOffsetBytes;
    artifact.pushConstantSize = result.bindings.pushConstantSizeBytes;
    auto& layout = result.bindings.layout;
    artifact.pushDataStart = layout.pushDataStartDword;
    artifact.stencilReference = layout.stencilReferenceDword;
    artifact.memoryOffset = layout.memoryOffsetDword;
    artifact.memoryOffsetCount = layout.memoryOffsetCount;
    artifact.dispatchThreadLimit = layout.dispatchThreadLimit;
    artifact.runtimeImageCount = layout.runtimeImageCount;
    artifact.runtimeStateDwords = layout.runtimeStateDwords;
    artifact.userRegisters = std::move(layout.userDataRegisters);
    for (auto& descriptor : layout.descriptors) artifact.descriptors.push_back({static_cast<std::uint32_t>(descriptor.kind), std::move(descriptor.resources)});
    artifact.buffers.assign(buffers.begin(), buffers.end());
    artifact.code = std::move(result.code);
    ValidateShaderArtifact(artifact);
    return artifact;
}

AnyBox::Shader::GraphicsArtifact AnyBox::Shader::MakeGraphicsArtifact(StageCompilation result, const GraphicsArtifactKey& key, std::uint32_t waveSize, std::uint32_t hostSubgroupSize, std::span<const BufferBinding> buffers) {
    const auto& info = result.info;
    if (info.fullSubgroups || info.functionLdsBuffer || info.meshWindowBuffer || info.splitStateLanes || info.scratchPoolSlotDwords) throw std::runtime_error("Graphics artifact auxiliary invocation storage is not implemented");
    GraphicsArtifact artifact;
    static_cast<ShaderArtifact&>(artifact) = MakeShaderArtifact(std::move(result), key.sourceHash, buffers, waveSize, hostSubgroupSize);
    artifact.stage = key.stage;
    artifact.layoutHash = key.layoutHash;
    ValidateGraphicsArtifact(artifact);
    return artifact;
}
