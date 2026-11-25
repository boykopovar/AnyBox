#include "ShaderBinary.hpp"
#include <algorithm>
#include <array>

void AnyBox::Shader::ValidateComputeArtifact(const ComputeArtifact& artifact) { ArtifactFormat::Validate(artifact); }

void AnyBox::Shader::ArtifactFormat::Validate(const ComputeArtifact& artifact) {
    ValidateShader(artifact);
    if ((artifact.waveSize != 32 && artifact.waveSize != 64) || (artifact.hostSubgroupSize != 32 && artifact.hostSubgroupSize != 64) || (artifact.executionFlags & ~(15u | NativeComputeInvocations)) != 0) throw std::invalid_argument("Shader artifact wave or execution flags are incompatible");
    if ((artifact.executionFlags & NativeComputeInvocations) != 0 && (artifact.waveSize != 64 || artifact.hostSubgroupSize != 32 || artifact.splitStateLanes != 0 || artifact.scratchPoolSlotDwords != 0)) throw std::invalid_argument("Shader artifact native invocation layout is incompatible");
    std::uint64_t threads = 1;
    for (auto size : artifact.threads) {
        if (size == 0 || size > 1024) throw std::invalid_argument("Shader artifact workgroup dimension is invalid");
        threads *= size;
    }
    if (threads > 1024 || artifact.sharedMemoryBytes % 4 != 0) throw std::invalid_argument("Shader artifact workgroup size or shared memory alignment is invalid");
    auto spanned = artifact.executionFlags & SpannedComputeWave;
    if (spanned != 0 && (spanned != SpannedComputeWave || artifact.waveSize != 64 || artifact.hostSubgroupSize != 32 || threads != 64 || (artifact.executionFlags & NativeComputeInvocations) != 0)) throw std::invalid_argument("Shader artifact spanned wave layout is incompatible");
}
