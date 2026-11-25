#pragma once

#include <anybox/shader/SerializedGraphicsCompiler.hpp>
#include <anybox/graphics/container/SerializedStage.hpp>
#include <anybox/graphics/container/SerializedGraphicsShader.hpp>

namespace AnyBox::Shader::SerializedGraphics {
SerializedGraphicsStageCompilation Finish(ShaderRecompiler::IrProgram& program, StageCompilation compiled, const Graphics::Container::SerializedStageMetadata& metadata, const Graphics::Container::RootSignatureLayout& root, std::uint32_t visibility);
}
