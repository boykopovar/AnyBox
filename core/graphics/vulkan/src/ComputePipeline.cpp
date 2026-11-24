#include <anybox/vulkan/ComputePipeline.hpp>

AnyBox::Vulkan::ComputePipeline::ComputePipeline(Context& context, std::span<const std::uint32_t> code, std::uint32_t pushConstantSize, const char* entryPoint, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::uint32_t requiredSubgroupSize) : context(context), pushConstantSize(pushConstantSize), descriptorSetCount(static_cast<std::uint32_t>(descriptorLayouts.size())) {
    if (code.size() < 5 || code.front() != 0x07230203 || !entryPoint || *entryPoint == '\0') throw std::invalid_argument("Vulkan compute shader code or entry point is invalid");
    if (pushConstantSize % 4 != 0 || pushConstantSize > context.GetProperties().limits.maxPushConstantsSize) throw std::invalid_argument("Vulkan compute push constant size is invalid");
    if (descriptorLayouts.size() > context.GetProperties().limits.maxBoundDescriptorSets) throw std::invalid_argument("Vulkan compute descriptor set count exceeds the device limit");
    for (auto value : descriptorLayouts) if (!value) throw std::invalid_argument("Vulkan compute descriptor set layout is null");
    if (requiredSubgroupSize && !context.CanRequireComputeSubgroupSize(requiredSubgroupSize)) throw std::invalid_argument("Required Vulkan compute subgroup size is unsupported");
    const auto& functions = context.GetFunctions();
    VkShaderModule module = VK_NULL_HANDLE;
    try {
        VkShaderModuleCreateInfo shader{};
        shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader.codeSize = code.size_bytes();
        shader.pCode = code.data();
        Check(functions.createShaderModule(context.GetDevice(), &shader, nullptr, &module), "Create Vulkan compute shader module");

        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetCount;
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
        layoutInfo.pPushConstantRanges = pushConstantSize == 0 ? nullptr : &range;
        Check(functions.createPipelineLayout(context.GetDevice(), &layoutInfo, nullptr, &layout), "Create Vulkan compute pipeline layout");

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup{};
        subgroup.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        subgroup.requiredSubgroupSize = requiredSubgroupSize;
        VkComputePipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        if (requiredSubgroupSize) info.stage.pNext = &subgroup;
        info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = module;
        info.stage.pName = entryPoint;
        info.layout = layout;
        Check(functions.createComputePipelines(context.GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline), "Create Vulkan compute pipeline");
    } catch (...) {
        if (module) functions.destroyShaderModule(context.GetDevice(), module, nullptr);
        release();
        throw;
    }
    functions.destroyShaderModule(context.GetDevice(), module, nullptr);
}

AnyBox::Vulkan::ComputePipeline::~ComputePipeline() { release(); }

void AnyBox::Vulkan::ComputePipeline::release() noexcept {
    const auto& functions = context.GetFunctions();
    if (pipeline) functions.destroyPipeline(context.GetDevice(), pipeline, nullptr);
    if (layout) functions.destroyPipelineLayout(context.GetDevice(), layout, nullptr);
}

void AnyBox::Vulkan::ComputePipeline::Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups, std::span<const VkDescriptorSet> descriptors) const {
    if (!commands || constants.size() != pushConstantSize) throw std::invalid_argument("Vulkan compute commands or push constants are invalid");
    if (descriptors.size() != descriptorSetCount) throw std::invalid_argument("Vulkan compute descriptor sets do not match the pipeline layout");
    for (auto value : descriptors) if (!value) throw std::invalid_argument("Vulkan compute descriptor set is null");
    const auto& limits = context.GetProperties().limits;
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (groups[index] > limits.maxComputeWorkGroupCount[index]) throw std::out_of_range("Vulkan compute dispatch exceeds the workgroup count limit");
    }
    const auto& functions = context.GetFunctions();
    functions.bindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    if (!descriptors.empty()) functions.bindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, descriptorSetCount, descriptors.data(), 0, nullptr);
    if (!constants.empty()) functions.pushConstants(commands, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, constants.data());
    functions.dispatch(commands, groups[0], groups[1], groups[2]);
}
