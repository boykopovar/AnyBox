#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/vulkan/ComputePipeline.hpp>

namespace AnyBox::Shader::Runtime {
class ComputePipeline {
public:
    static constexpr std::uint32_t MemoryTableBinding = 59;
    static constexpr std::uint32_t FaultBufferBinding = 60;
    static constexpr std::uint32_t ShaderDataBinding = 62;
    ComputePipeline(Vulkan::Context& context, const ComputeArtifact& artifact);
    ~ComputePipeline();
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    const Vulkan::ComputePipeline& GetPipeline() const noexcept { return *pipeline; }
    VkDescriptorSetLayout GetDescriptorLayout() const noexcept { return descriptorLayout; }
    Vulkan::Context& GetContext() const noexcept { return context; }
    std::uint32_t GetDescriptorMask() const noexcept { return descriptorMask; }
    std::uint32_t GetShaderDataSize() const noexcept { return shaderDataSize; }
    bool RequiresResources() const noexcept { return resources; }
    std::span<const VkDescriptorSetLayoutBinding> GetDescriptorBindings() const noexcept { return bindings; }

private:
    Vulkan::Context& context;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    std::uint32_t descriptorMask = 0;
    std::uint32_t shaderDataSize = 0;
    bool resources = false;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::unique_ptr<Vulkan::ComputePipeline> pipeline;
};
}
