#pragma once

#include "Context.hpp"
#include <span>

namespace AnyBox::Vulkan {
class PipelineLayout {
public:
    PipelineLayout(Context& context, std::span<const VkPushConstantRange> constants = {}, std::span<const VkDescriptorSetLayout> descriptors = {});
    ~PipelineLayout();
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;
    VkPipelineLayout GetHandle() const noexcept { return layout; }

private:
    Context& context;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};
}
