#include "Binary.hpp"

std::vector<std::byte> AnyBox::Shader::EncodeGraphicsArtifact(const GraphicsArtifact& artifact) {
    ValidateGraphicsArtifact(artifact);
    ArtifactFormat::Writer payload;
    payload.Word(static_cast<std::uint32_t>(artifact.stage));
    payload.Bytes(artifact.layoutHash);
    ArtifactFormat::WriteShaderMetadata(payload, artifact);
    ArtifactFormat::WriteShaderResources(payload, artifact);
    auto bytes = payload.Take();
    if (bytes.size() > UINT32_MAX) throw std::length_error("Graphics artifact payload exceeds its size field");
    ArtifactFormat::Writer output;
    output.Bytes(GraphicsFormat::Magic);
    output.Word(1);
    output.Word(static_cast<std::uint32_t>(bytes.size()));
    output.Bytes(artifact.sourceHash);
    output.Bytes(HashShader(bytes));
    output.Bytes(bytes);
    return output.Take();
}
