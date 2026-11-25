#include "Compilation.hpp"
#include <anybox/shader/ShaderArtifactBuilder.hpp>
#include <RuntimeAbi.hpp>
#include <algorithm>

AnyBox::Shader::ComputeArtifact AnyBox::Shader::Tools::MakeArtifact(ComputeCompilation result, std::span<const BufferBinding> buffers, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderDigest& source) {
    static_assert(ShaderRecompiler::RuntimeAbi::Version == ComputeArtifact{}.runtimeVersion);
    ComputeArtifact artifact;
    auto info = result.info;
    static_cast<ShaderArtifact&>(artifact) = MakeShaderArtifact(std::move(result), source, buffers, input.waveSize, input.hostSubgroupSize);
    std::copy(std::begin(input.threadsNum), std::end(input.threadsNum), artifact.threads.begin());
    artifact.sharedMemoryBytes = input.ldsSizeDwords * 4;
    artifact.executionFlags = static_cast<std::uint32_t>(info.fullSubgroups) | (static_cast<std::uint32_t>(info.functionLdsBuffer) << 1) | (static_cast<std::uint32_t>(info.meshWindowBuffer) << 2) | (static_cast<std::uint32_t>(info.anySubgroupSize) << 3);
    artifact.splitStateLanes = info.splitStateLanes;
    artifact.scratchPoolSlotDwords = info.scratchPoolSlotDwords;
    return artifact;
}
