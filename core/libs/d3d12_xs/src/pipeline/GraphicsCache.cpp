#include "Cache.hpp"

AnyBox::Shader::GraphicsArtifact AnyBox::Graphics::LoadPreparedGraphics(const Shader::GraphicsArtifactKey& expected) {
    auto name = Shader::DigestName(Shader::HashGraphicsArtifactKey(expected)) + ".abxg";
    auto bytes = ReadPreparedArtifact(name, "Prepared graphics shader");
    return Shader::DecodeGraphicsArtifact(bytes, expected);
}
