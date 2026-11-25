#pragma once

#include "Context.hpp"

namespace AnyBox::Vulkan {
class GraphicsPipeline {
public:
    GraphicsPipeline(Context& context, const VkGraphicsPipelineCreateInfo& description);
    ~GraphicsPipeline();
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    VkPipeline GetHandle() const noexcept { return pipeline; }

private:
    Context& context;
    VkPipeline pipeline = VK_NULL_HANDLE;
};
}
