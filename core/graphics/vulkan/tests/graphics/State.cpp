#include "State.hpp"

AnyBox::Vulkan::Tests::GraphicsState::GraphicsState(VkPipelineLayout layout, VkRenderPass pass, VkShaderModule vertex, VkShaderModule pixel, VkExtent2D extent) {
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[0].module = vertex;
    stages[1].module = pixel;
    stages[0].pName = stages[1].pName = "main";
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    viewport = {0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1};
    scissor.extent = extent;
    viewports.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewports.viewportCount = viewports.scissorCount = 1;
    viewports.pViewports = &viewport;
    viewports.pScissors = &scissor;
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1;
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &color;
    description.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    description.stageCount = static_cast<std::uint32_t>(stages.size());
    description.pStages = stages.data();
    description.pVertexInputState = &vertexInput;
    description.pInputAssemblyState = &assembly;
    description.pViewportState = &viewports;
    description.pRasterizationState = &rasterizer;
    description.pMultisampleState = &multisample;
    description.pColorBlendState = &blend;
    description.layout = layout;
    description.renderPass = pass;
    description.basePipelineIndex = -1;
}
