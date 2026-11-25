#include "Binary.hpp"

std::vector<std::byte> AnyBox::Shader::EncodeResourcePlan(const ShaderRecompiler::IrResourcePlan& plan, const ShaderDigest& source) {
    PlanFormat::Validate(plan);
    std::vector<std::byte> encoded;
    if (!ShaderRecompiler::ShaderDiskCache::EncodePlan(plan, encoded)) throw std::invalid_argument("Prepared resource plan contains unresolved references");
    ArtifactFormat::Writer payload;
    payload.Word(ResourcePlanFormatVersion);
    payload.Word(static_cast<std::uint32_t>(plan.stage));
    payload.Bytes(encoded);
    auto bytes = payload.Take();
    if (bytes.size() > UINT32_MAX) throw std::length_error("Prepared resource plan payload exceeds its size field");
    ArtifactFormat::Writer output;
    output.Bytes(PlanFormat::Magic);
    output.Word(1);
    output.Word(static_cast<std::uint32_t>(bytes.size()));
    output.Bytes(source);
    output.Bytes(HashShader(bytes));
    output.Bytes(bytes);
    return output.Take();
}
