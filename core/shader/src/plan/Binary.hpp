#pragma once

#include <anybox/shader/ResourcePlan.hpp>
#include "../artifact/Binary.hpp"
#include <ShaderDiskCache.hpp>

namespace AnyBox::Shader::PlanFormat {
inline constexpr std::array<std::byte, 8> Magic{std::byte{'A'}, std::byte{'B'}, std::byte{'X'}, std::byte{'R'}, std::byte{'P'}, std::byte{'0'}, std::byte{'0'}, std::byte{'1'}};
static_assert(ShaderRecompiler::ShaderDiskCache::FormatVersion == ResourcePlanFormatVersion);

inline bool IsStage(ShaderRecompiler::IrShaderStage stage) noexcept {
    return stage >= ShaderRecompiler::IrShaderStage::Vertex && stage <= ShaderRecompiler::IrShaderStage::TessellationEvaluation;
}

inline void Validate(const ShaderRecompiler::IrResourcePlan& plan) {
    if (!IsStage(plan.stage)) throw std::invalid_argument("Prepared resource plan shader stage is invalid");
    if (!plan.srtPlanComplete || !plan.resourceTrackingComplete) throw std::invalid_argument("Prepared resource plan is incomplete");
    if (plan.userDataBase > 128 || plan.userDataCount > 128 - plan.userDataBase) throw std::invalid_argument("Prepared resource plan user data exceeds the artifact ABI capacity");
}
}
