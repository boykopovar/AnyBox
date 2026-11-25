#pragma once

#include <anybox/shader/GraphicsArtifact.hpp>
#include "../ShaderBinary.hpp"

namespace AnyBox::Shader::GraphicsFormat {
inline constexpr std::array<std::byte, 8> Magic{std::byte{'A'}, std::byte{'B'}, std::byte{'X'}, std::byte{'G'}, std::byte{'S'}, std::byte{'0'}, std::byte{'0'}, std::byte{'1'}};
inline bool IsStage(GraphicsArtifactStage stage) noexcept { return stage == GraphicsArtifactStage::Vertex || stage == GraphicsArtifactStage::Pixel; }
}
