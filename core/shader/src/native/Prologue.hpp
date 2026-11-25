#pragma once

#include <RdnaDecoder/RdnaProgram.hpp>
#include <span>
#include <vector>

namespace AnyBox::Shader {
bool NormalizeNativeHashPrologue(std::span<const std::uint32_t> code, std::vector<std::uint32_t>& normalized);
void RestoreNativeHashPrologue(ShaderRecompiler::RdnaProgram& program, std::span<const std::uint32_t> code);
}
