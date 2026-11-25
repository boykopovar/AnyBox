#pragma once

#include "../stream/Graphics.hpp"
#include <anybox/vulkan/GraphicsPipeline.hpp>
#include <anybox/vulkan/RenderPass.hpp>
#include <memory>

namespace AnyBox::Graphics {
class GraphicsFixedState {
public:
    GraphicsFixedState(Vulkan::Context& context, const GraphicsPipelineDescription& source);
    GraphicsFixedState(const GraphicsFixedState&) = delete;
    GraphicsFixedState& operator=(const GraphicsFixedState&) = delete;
    const VkGraphicsPipelineCreateInfo& GetDescription() const noexcept { return description; }
    void SetProgram(VkPipelineLayout layout, std::span<const VkPipelineShaderStageCreateInfo> stages);
    void SetTopology(VkPrimitiveTopology topology) noexcept { assembly.topology = topology; }
    const Vulkan::RenderPass& GetRenderPass() const noexcept { return *pass; }

private:
    Vulkan::Context& context;
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    VkPipelineViewportStateCreateInfo viewports{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisample{};
    VkPipelineColorBlendStateCreateInfo blend{};
    VkPipelineDepthStencilStateCreateInfo depth{};
    VkPipelineDynamicStateCreateInfo dynamic{};
    std::array<VkPipelineColorBlendAttachmentState, 8> colors{};
    std::array<VkDynamicState, 4> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_BLEND_CONSTANTS, VK_DYNAMIC_STATE_STENCIL_REFERENCE};
    VkSampleMask sampleMask = UINT32_MAX;
    std::unique_ptr<Vulkan::RenderPass> pass;
    VkGraphicsPipelineCreateInfo description{};
    void initializeRasterizer(const D3D12_RASTERIZER_DESC& source);
    void initializeBlend(const D3D12_BLEND_DESC& source, std::uint32_t count);
    void initializeDepth(const D3D12_DEPTH_STENCIL_DESC& source, BOOL bounds);
    void initializePass(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& source);
};
}
