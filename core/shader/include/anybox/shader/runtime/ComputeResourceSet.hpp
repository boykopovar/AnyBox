#pragma once

#include "ComputePipeline.hpp"

namespace AnyBox::Shader::Runtime {
struct ComputeBufferDescriptors {
    std::uint32_t binding;
    std::span<const VkDescriptorBufferInfo> values;
};

struct ComputeImageDescriptors {
    std::uint32_t binding;
    std::span<const VkDescriptorImageInfo> values;
};

class ComputeResourceSet {
public:
    ComputeResourceSet(const ComputePipeline& pipeline, std::span<const ComputeBufferDescriptors> buffers, std::span<const ComputeImageDescriptors> images);
    ~ComputeResourceSet();
    ComputeResourceSet(const ComputeResourceSet&) = delete;
    ComputeResourceSet& operator=(const ComputeResourceSet&) = delete;
    std::span<const VkDescriptorSet> GetSets() const noexcept { return {&set, set ? 1u : 0u}; }

private:
    Vulkan::Context& context;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};
}
