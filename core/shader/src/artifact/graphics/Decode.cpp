#include "Binary.hpp"
#include <algorithm>

AnyBox::Shader::GraphicsArtifact AnyBox::Shader::DecodeGraphicsArtifact(std::span<const std::byte> data, const GraphicsArtifactKey& expected) {
    if (!GraphicsFormat::IsStage(expected.stage)) throw std::invalid_argument("Graphics artifact key shader stage is invalid");
    ArtifactFormat::Reader input(data);
    if (!std::ranges::equal(input.Bytes(GraphicsFormat::Magic.size()), GraphicsFormat::Magic) || input.Word() != 1) throw std::invalid_argument("Graphics artifact magic or version is invalid");
    auto size = input.Word();
    if (!std::ranges::equal(input.Bytes(expected.sourceHash.size()), expected.sourceHash)) throw std::invalid_argument("Graphics artifact source bytecode digest does not match");
    auto hash = input.Bytes(32);
    auto payload = input.Bytes(size);
    if (!input.Empty() || !std::ranges::equal(hash, HashShader(payload))) throw std::invalid_argument("Graphics artifact payload digest or size does not match");
    ArtifactFormat::Reader reader(payload);
    GraphicsArtifact artifact;
    artifact.sourceHash = expected.sourceHash;
    artifact.stage = static_cast<GraphicsArtifactStage>(reader.Word());
    auto layout = reader.Bytes(expected.layoutHash.size());
    if (artifact.stage != expected.stage || !std::ranges::equal(layout, expected.layoutHash)) throw std::invalid_argument("Graphics artifact stage or pipeline layout digest does not match");
    artifact.layoutHash = expected.layoutHash;
    ArtifactFormat::ReadShaderMetadata(reader, artifact);
    ArtifactFormat::ReadShaderResources(reader, artifact);
    if (!reader.Empty()) throw std::invalid_argument("Graphics artifact payload has trailing data");
    ValidateGraphicsArtifact(artifact);
    return artifact;
}
