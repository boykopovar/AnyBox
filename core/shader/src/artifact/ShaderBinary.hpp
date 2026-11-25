#pragma once

#include "Binary.hpp"

namespace AnyBox::Shader::ArtifactFormat {
void ValidateShader(const ShaderArtifact& artifact);
void WriteShaderMetadata(Writer& writer, const ShaderArtifact& artifact);
void ReadShaderMetadata(Reader& reader, ShaderArtifact& artifact);
void WriteShaderResources(Writer& writer, const ShaderArtifact& artifact);
void ReadShaderResources(Reader& reader, ShaderArtifact& artifact);
}
