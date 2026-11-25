#pragma once

#include <anybox/shader/GraphicsArtifact.hpp>

namespace AnyBox::Graphics {
std::vector<std::byte> ReadPreparedArtifact(const std::string& name, const char* role);
Shader::GraphicsArtifact LoadPreparedGraphics(const Shader::GraphicsArtifactKey& expected);
}
