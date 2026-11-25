#pragma once

#include <anybox/shader/GraphicsLayout.hpp>
#include <filesystem>
#include <string_view>

namespace AnyBox::Shader::Tools {
struct GraphicsOptions {
    std::filesystem::path vertex;
    std::filesystem::path pixel;
    std::filesystem::path root;
    std::filesystem::path output;
    GraphicsShaderLayout layout;
};
GraphicsOptions ParseGraphicsOptions(std::span<const std::string_view> arguments);
void PrepareGraphics(const GraphicsOptions& options, bool verify);
}
