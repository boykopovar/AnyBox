#pragma once

#include <anybox/vulkan/GraphicsPipeline.hpp>
#include <array>

namespace AnyBox::Vulkan::Tests {
class GraphicsState {
public:
    GraphicsState(VkPipelineLayout layout, VkRenderPass pass, VkShaderModule vertex, VkShaderModule pixel, VkExtent2D extent);
    GraphicsState(const GraphicsState&) = delete;
    GraphicsState& operator=(const GraphicsState&) = delete;
    const VkGraphicsPipelineCreateInfo& GetDescription() const noexcept { return description; }

private:
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    VkViewport viewport{};
    VkRect2D scissor{};
    VkPipelineViewportStateCreateInfo viewports{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisample{};
    VkPipelineColorBlendAttachmentState color{};
    VkPipelineColorBlendStateCreateInfo blend{};
    VkGraphicsPipelineCreateInfo description{};
};
}
