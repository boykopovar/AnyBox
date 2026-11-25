#include "Binary.hpp"

void AnyBox::Shader::ValidateGraphicsArtifact(const GraphicsArtifact& artifact) {
    ArtifactFormat::ValidateShader(artifact);
    if (!GraphicsFormat::IsStage(artifact.stage)) throw std::invalid_argument("Graphics artifact shader stage is invalid");
    if (artifact.dispatchThreadLimit) throw std::invalid_argument("Graphics artifact cannot declare compute dispatch limits");
    std::uint32_t entries = 0;
    const auto& code = artifact.code;
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        if ((code[offset] & 65535) != 15) continue;
        ++entries;
        if ((code[offset] >> 16) < 5 || code[offset + 1] != static_cast<std::uint32_t>(artifact.stage) || code[offset + 3] != 0x6E69616D || code[offset + 4] != 0) throw std::invalid_argument("Graphics artifact SPIR-V entry point disagrees with its shader stage");
    }
    if (entries != 1) throw std::invalid_argument("Graphics artifact must contain one shader entry point");
}

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashGraphicsArtifactKey(const GraphicsArtifactKey& key) {
    if (!GraphicsFormat::IsStage(key.stage)) throw std::invalid_argument("Graphics artifact key shader stage is invalid");
    ArtifactFormat::Writer writer;
    writer.Bytes(GraphicsFormat::Magic);
    writer.Word(static_cast<std::uint32_t>(key.stage));
    writer.Bytes(key.sourceHash);
    writer.Bytes(key.layoutHash);
    return HashShader(writer.Take());
}
