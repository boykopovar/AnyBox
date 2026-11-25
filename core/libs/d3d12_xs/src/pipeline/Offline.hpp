#pragma once

#include "../root/Signature.hpp"
#include <anybox/shader/RootArgumentWriter.hpp>
#include <anybox/shader/runtime/ComputePipeline.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <memory>
#include <optional>

namespace AnyBox::Graphics {
class PipelineState;
class Resource;

class OfflinePipeline {
public:
    OfflinePipeline(Device& device, const RootSignature& root, Shader::ComputeArtifact artifact) : plan(artifact.buffers, root.GetLayout()), writer(plan, artifact.userRegisters), pipeline(device.GetGpuContext(), artifact), pushConstantSize(artifact.pushConstantSize), pushDataStart(artifact.pushDataStart), descriptorCount(artifact.descriptors.size()) {}
    const Shader::Runtime::ComputePipeline& GetPipeline() const noexcept { return pipeline; }
    const Shader::RootArgumentWriter& GetArgumentWriter() const noexcept { return writer; }
    std::uint32_t GetPushConstantSize() const noexcept { return pushConstantSize; }
    std::uint32_t GetPushDataStart() const noexcept { return pushDataStart; }
    bool SupportsPlacedUavs() const noexcept { return descriptorCount == 2; }

private:
    Shader::RootArgumentPlan plan;
    Shader::RootArgumentWriter writer;
    Shader::Runtime::ComputePipeline pipeline;
    std::uint32_t pushConstantSize;
    std::uint32_t pushDataStart;
    std::size_t descriptorCount;
};

class PreparedDispatch {
public:
    std::shared_ptr<const void> Record(VkCommandBuffer commands) const;

private:
    friend std::shared_ptr<const PreparedDispatch> PrepareComputeDispatch(std::shared_ptr<PipelineState> pipeline, const Commands::RootState& arguments, const std::array<std::uint32_t, 3>& groups);
    PreparedDispatch(std::shared_ptr<PipelineState> pipeline, std::vector<std::shared_ptr<Resource>> buffers, Vulkan::Buffer table, std::vector<std::uint32_t> constants, const std::array<std::uint32_t, 3>& groups);

    std::shared_ptr<PipelineState> pipeline;
    std::vector<std::shared_ptr<Resource>> buffers;
    Vulkan::Buffer table;
    std::vector<std::uint32_t> constants;
    std::array<std::uint32_t, 3> groups;
};

std::shared_ptr<const PreparedDispatch> PrepareComputeDispatch(std::shared_ptr<PipelineState> pipeline, const Commands::RootState& arguments, const std::array<std::uint32_t, 3>& groups);
std::optional<Shader::ComputeArtifact> FindPreparedCompute(std::span<const std::byte> code);
Shader::ComputeArtifact LoadPreparedCompute(std::span<const std::byte> code);
bool RequiresPreparedCompute(Device& device, std::span<const std::byte> code);
}
