#include "ShaderBinary.hpp"

void AnyBox::Shader::ArtifactFormat::ReadShaderResources(Reader& reader, ShaderArtifact& artifact) {
    artifact.userRegisters = reader.Words();
    auto descriptorCount = reader.Count(2);
    if (descriptorCount > 258) throw std::length_error("Shader artifact descriptor count exceeds its limit");
    for (std::uint32_t index = 0; index < descriptorCount; ++index) {
        auto kind = reader.Word();
        artifact.descriptors.push_back({kind, reader.Words()});
    }
    auto bufferCount = reader.Count(9);
    if (bufferCount > 128) throw std::length_error("Shader artifact buffer count exceeds its limit");
    for (std::uint32_t index = 0; index < bufferCount; ++index) {
        auto category = reader.Word();
        BufferBinding buffer{static_cast<ResourceClass>(category), reader.Word(), reader.Word(), reader.Word(), reader.Word(), BufferKind::Raw};
        auto kind = reader.Word();
        if (category > 2 || kind > 2) throw std::invalid_argument("Shader artifact resource class or buffer kind is invalid");
        buffer.kind = static_cast<BufferKind>(kind);
        buffer.stride = reader.Word();
        buffer.byteSize = reader.Word();
        auto flags = reader.Word();
        if ((flags & ~7u) != 0) throw std::invalid_argument("Shader artifact buffer flags are invalid");
        buffer.globallyCoherent = (flags & 1) != 0;
        buffer.hasCounter = (flags & 2) != 0;
        buffer.rasterizerOrdered = (flags & 4) != 0;
        artifact.buffers.push_back(buffer);
    }
    artifact.code = reader.Words();
}
