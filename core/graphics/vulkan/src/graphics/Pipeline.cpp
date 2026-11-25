#include <anybox/vulkan/GraphicsPipeline.hpp>

AnyBox::Vulkan::GraphicsPipeline::GraphicsPipeline(Context& context, const VkGraphicsPipelineCreateInfo& description) : context(context) {
    if (description.sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO || !description.layout || !description.renderPass || description.stageCount == 0 || !description.pStages) throw std::invalid_argument("Vulkan graphics pipeline description is incomplete");
    if (description.pNext || description.flags != 0 || description.pTessellationState || description.basePipelineHandle) throw std::runtime_error("Vulkan graphics pipeline extension or derivation mode is not implemented");
    if (!description.pVertexInputState || !description.pInputAssemblyState || !description.pRasterizationState) throw std::invalid_argument("Vulkan graphics pipeline fixed state is incomplete");
    if (!description.pRasterizationState->rasterizerDiscardEnable && (!description.pViewportState || !description.pMultisampleState)) throw std::invalid_argument("Vulkan graphics rasterization state is incomplete");
    VkShaderStageFlags stages = 0;
    for (std::uint32_t index = 0; index < description.stageCount; ++index) {
        const auto& stage = description.pStages[index];
        if (stage.stage != VK_SHADER_STAGE_VERTEX_BIT && stage.stage != VK_SHADER_STAGE_FRAGMENT_BIT) throw std::runtime_error("Vulkan graphics shader stage is not implemented");
        if (stage.sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO || !stage.module || !stage.pName || !*stage.pName || (stages & stage.stage) != 0) throw std::invalid_argument("Vulkan graphics shader stage is incomplete or duplicated");
        if (stage.pNext || stage.flags != 0) throw std::runtime_error("Vulkan graphics shader stage extension is not implemented");
        stages |= stage.stage;
    }
    if ((stages & VK_SHADER_STAGE_VERTEX_BIT) == 0) throw std::invalid_argument("Vulkan graphics pipeline has no vertex stage");
    auto result = context.GetFunctions().createGraphicsPipelines(context.GetDevice(), VK_NULL_HANDLE, 1, &description, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
        if (pipeline) context.GetFunctions().destroyPipeline(context.GetDevice(), pipeline, nullptr);
        Check(result, "Create Vulkan graphics pipeline");
    }
}

AnyBox::Vulkan::GraphicsPipeline::~GraphicsPipeline() {
    context.GetFunctions().destroyPipeline(context.GetDevice(), pipeline, nullptr);
}
