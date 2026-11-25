#include "ShaderBinary.hpp"
#include <algorithm>

using namespace AnyBox::Shader;
using namespace AnyBox::Shader::ArtifactFormat;

ComputeArtifact AnyBox::Shader::DecodeComputeArtifact(std::span<const std::byte> data, const ShaderDigest& expectedSource) {
    Reader input(data);
    if (!std::ranges::equal(input.Bytes(Magic.size()), Magic) || input.Word() != 2) throw std::invalid_argument("Shader artifact magic or version is invalid");
    auto size = input.Word();
    if (!std::ranges::equal(input.Bytes(expectedSource.size()), expectedSource)) throw std::invalid_argument("Shader artifact does not match its source bytecode");
    auto hash = input.Bytes(32);
    auto payload = input.Bytes(size);
    if (!input.Empty()) throw std::invalid_argument("Shader artifact has trailing data");
    if (!std::ranges::equal(hash, HashShader(payload))) throw std::invalid_argument("Shader artifact payload digest does not match");
    Reader reader(payload);
    ComputeArtifact artifact;
    artifact.sourceHash = expectedSource;
    artifact.profile = reader.Word();
    artifact.runtimeVersion = reader.Word();
    artifact.bdaVersion = reader.Word();
    for (auto& value : artifact.threads) value = reader.Word();
    artifact.waveSize = reader.Word();
    artifact.sharedMemoryBytes = reader.Word();
    artifact.pushConstantOffset = reader.Word();
    artifact.pushConstantSize = reader.Word();
    artifact.pushDataStart = reader.Word();
    artifact.stencilReference = reader.Word();
    artifact.memoryOffset = reader.Word();
    artifact.memoryOffsetCount = reader.Word();
    auto limit = reader.Word();
    if (limit > 1) throw std::invalid_argument("Shader artifact dispatch limit flag is invalid");
    artifact.dispatchThreadLimit = limit != 0;
    artifact.runtimeImageCount = reader.Word();
    artifact.runtimeStateDwords = reader.Word();
    artifact.executionFlags = reader.Word();
    artifact.splitStateLanes = reader.Word();
    artifact.scratchPoolSlotDwords = reader.Word();
    artifact.hostSubgroupSize = reader.Word();
    ReadShaderResources(reader, artifact);
    if (!reader.Empty()) throw std::invalid_argument("Shader artifact payload has trailing data");
    Validate(artifact);
    return artifact;
}
