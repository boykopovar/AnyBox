#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/shader/ResourcePlan.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>

namespace AnyBox::Graphics {
struct PreparedNativeCompute {
    Shader::ComputeArtifact shader;
    ShaderRecompiler::IrResourcePlan resources;
};

PreparedNativeCompute LoadPreparedNativeCompute(const Shader::SerializedComputeSource& source, const Shader::ShaderDigest& root, std::uint32_t subgroupSize);
}
