#pragma once

#include "Context.hpp"
#include <array>
#include <span>

namespace AnyBox::Vulkan {
class ComputePipeline {
public:
    ComputePipeline(Context& context, std::span<const std::uint32_t> code, std::uint32_t pushConstantSize, const char* entryPoint = "main", std::span<const VkDescriptorSetLayout> descriptorLayouts = {}, std::uint32_t requiredSubgroupSize = 0);
    ~ComputePipeline();
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    void Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups, std::span<const VkDescriptorSet> descriptors = {}) const;

private:
    void release() noexcept;
    Context& context;
    std::uint32_t pushConstantSize;
    std::uint32_t descriptorSetCount;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};
}
