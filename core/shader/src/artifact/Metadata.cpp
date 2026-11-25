#include "ShaderBinary.hpp"

void AnyBox::Shader::ArtifactFormat::WriteShaderMetadata(Writer& writer, const ShaderArtifact& artifact) {
    for (auto value : {artifact.profile, artifact.runtimeVersion, artifact.bdaVersion, artifact.waveSize, artifact.hostSubgroupSize, artifact.pushConstantOffset, artifact.pushConstantSize, artifact.pushDataStart, artifact.stencilReference, artifact.memoryOffset, artifact.memoryOffsetCount, static_cast<std::uint32_t>(artifact.dispatchThreadLimit), artifact.runtimeImageCount, artifact.runtimeStateDwords}) writer.Word(value);
}

void AnyBox::Shader::ArtifactFormat::ReadShaderMetadata(Reader& reader, ShaderArtifact& artifact) {
    artifact.profile = reader.Word();
    artifact.runtimeVersion = reader.Word();
    artifact.bdaVersion = reader.Word();
    artifact.waveSize = reader.Word();
    artifact.hostSubgroupSize = reader.Word();
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
}
