#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/shader/ResourcePlan.hpp>
#include <filesystem>

namespace AnyBox::Shader::Tools {
struct PreparedSerializedCompute {
    ComputeArtifact shader;
    ShaderRecompiler::IrResourcePlan resources;
};

PreparedSerializedCompute CompileSerializedCompute(std::span<const std::byte> data, std::span<const std::byte> root, std::uint32_t subgroupSize);
void PrepareSerializedCompute(const std::filesystem::path& input, const std::filesystem::path& root, const std::filesystem::path& directory, std::uint32_t subgroupSize);
void CheckSerializedComputeGpu(const std::filesystem::path& input, const std::filesystem::path& root, const std::filesystem::path& directory, std::uint32_t subgroupSize);
}
