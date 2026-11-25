#pragma once

#include "ShaderArtifact.hpp"
#include <IntermediateRepresentation/IrMetadata/ResourcePlan.hpp>

namespace AnyBox::Shader {
inline constexpr std::uint32_t ResourcePlanFormatVersion = 96;

std::vector<std::byte> EncodeResourcePlan(const ShaderRecompiler::IrResourcePlan& plan, const ShaderDigest& source);
ShaderRecompiler::IrResourcePlan DecodeResourcePlan(std::span<const std::byte> data, const ShaderDigest& source, ShaderRecompiler::IrShaderStage stage);
}
