#include <anybox/vulkan/PipelineLayout.hpp>

AnyBox::Vulkan::PipelineLayout::PipelineLayout(Context& context, std::span<const VkPushConstantRange> constants, std::span<const VkDescriptorSetLayout> descriptors) : context(context) {
    const auto& limits = context.GetProperties().limits;
    if (constants.size() > UINT32_MAX || descriptors.size() > limits.maxBoundDescriptorSets) throw std::length_error("Vulkan pipeline layout exceeds device limits");
    for (auto descriptor : descriptors) if (!descriptor) throw std::invalid_argument("Vulkan pipeline descriptor layout is null");
    VkShaderStageFlags stages = 0;
    for (const auto& range : constants) {
        if (range.stageFlags == 0 || (stages & range.stageFlags) != 0) throw std::invalid_argument("Vulkan push constant stages are empty or duplicated");
        if (range.size == 0 || (range.offset | range.size) % 4 != 0 || range.offset > limits.maxPushConstantsSize || range.size > limits.maxPushConstantsSize - range.offset) throw std::invalid_argument("Vulkan push constant range exceeds device limits");
        stages |= range.stageFlags;
    }
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<std::uint32_t>(descriptors.size());
    info.pSetLayouts = descriptors.data();
    info.pushConstantRangeCount = static_cast<std::uint32_t>(constants.size());
    info.pPushConstantRanges = constants.data();
    Check(context.GetFunctions().createPipelineLayout(context.GetDevice(), &info, nullptr, &layout), "Create Vulkan pipeline layout");
}

AnyBox::Vulkan::PipelineLayout::~PipelineLayout() {
    context.GetFunctions().destroyPipelineLayout(context.GetDevice(), layout, nullptr);
}
