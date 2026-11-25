#include "ShaderBinary.hpp"

void AnyBox::Shader::ArtifactFormat::WriteShaderResources(Writer& writer, const ShaderArtifact& artifact) {
    writer.Words(artifact.userRegisters);
    writer.Word(static_cast<std::uint32_t>(artifact.descriptors.size()));
    for (const auto& descriptor : artifact.descriptors) {
        writer.Word(descriptor.kind);
        writer.Words(descriptor.resources);
    }
    writer.Word(static_cast<std::uint32_t>(artifact.buffers.size()));
    for (const auto& buffer : artifact.buffers) {
        for (auto value : {static_cast<std::uint32_t>(buffer.resourceClass), buffer.rangeId, buffer.space, buffer.firstRegister, buffer.count, static_cast<std::uint32_t>(buffer.kind), buffer.stride, buffer.byteSize}) writer.Word(value);
        writer.Word(static_cast<std::uint32_t>(buffer.globallyCoherent) | (static_cast<std::uint32_t>(buffer.hasCounter) << 1) | (static_cast<std::uint32_t>(buffer.rasterizerOrdered) << 2));
    }
    writer.Words(artifact.code);
}
