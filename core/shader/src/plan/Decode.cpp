#include "Binary.hpp"
#include <algorithm>

ShaderRecompiler::IrResourcePlan AnyBox::Shader::DecodeResourcePlan(std::span<const std::byte> data, const ShaderDigest& source, ShaderRecompiler::IrShaderStage stage) {
    if (!PlanFormat::IsStage(stage)) throw std::invalid_argument("Prepared resource plan expected shader stage is invalid");
    ArtifactFormat::Reader input(data);
    if (!std::ranges::equal(input.Bytes(PlanFormat::Magic.size()), PlanFormat::Magic) || input.Word() != 1) throw std::invalid_argument("Prepared resource plan magic or version is invalid");
    auto size = input.Word();
    if (!std::ranges::equal(input.Bytes(source.size()), source)) throw std::invalid_argument("Prepared resource plan source digest does not match");
    auto hash = input.Bytes(32);
    auto payload = input.Bytes(size);
    if (!input.Empty() || !std::ranges::equal(hash, HashShader(payload))) throw std::invalid_argument("Prepared resource plan payload digest or size does not match");
    ArtifactFormat::Reader reader(payload);
    if (reader.Word() != ResourcePlanFormatVersion || reader.Word() != static_cast<std::uint32_t>(stage)) throw std::invalid_argument("Prepared resource plan encoding version or stage does not match");
    ShaderRecompiler::IrResourcePlan plan;
    if (!ShaderRecompiler::ShaderDiskCache::DecodePlan(payload.subspan(8), plan)) throw std::invalid_argument("Prepared resource plan payload is invalid");
    PlanFormat::Validate(plan);
    if (plan.stage != stage) throw std::invalid_argument("Prepared resource plan internal shader stage does not match");
    return plan;
}
