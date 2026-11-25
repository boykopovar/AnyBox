#pragma once

#include <SpirvBackend/SpirvEmitter.hpp>

namespace AnyBox::Shader {
void ValidateGraphicsWave(std::span<const std::uint32_t> code, std::uint32_t waveSize, const ShaderRecompiler::SpirvTargetOptions& target);
}
