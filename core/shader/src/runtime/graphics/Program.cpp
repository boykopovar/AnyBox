#include <anybox/shader/runtime/GraphicsProgram.hpp>
#include "Requirements.hpp"
#include <algorithm>

AnyBox::Shader::Runtime::GraphicsProgram::GraphicsProgram(Vulkan::Context& context, const GraphicsArtifact& vertex, const GraphicsArtifact* pixel) : context(context) {
    if (vertex.stage != GraphicsArtifactStage::Vertex || (pixel && pixel->stage != GraphicsArtifactStage::Pixel)) throw std::invalid_argument("Offline graphics program shader stages are invalid");
    if (pixel && (vertex.layoutHash != pixel->layoutHash || vertex.profile != pixel->profile || vertex.hostSubgroupSize != pixel->hostSubgroupSize)) throw std::invalid_argument("Offline graphics program shader configurations disagree");
    CheckGraphicsRequirements(context, vertex);
    if (pixel) CheckGraphicsRequirements(context, *pixel);
    addStage(vertex);
    if (pixel) addStage(*pixel);
    try {
        std::vector<VkDescriptorSetLayout> sets;
        if (!bindings.empty()) {
            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = static_cast<std::uint32_t>(bindings.size());
            info.pBindings = bindings.data();
            Vulkan::Check(context.GetFunctions().createDescriptorSetLayout(context.GetDevice(), &info, nullptr, &descriptorLayout), "Create offline graphics descriptor layout");
            sets.push_back(descriptorLayout);
        }
        layout = std::make_unique<Vulkan::PipelineLayout>(context, constants, sets);
    } catch (...) {
        if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
        throw;
    }
}

void AnyBox::Shader::Runtime::GraphicsProgram::addStage(const GraphicsArtifact& artifact) {
    auto module = std::make_unique<Vulkan::ShaderModule>(context, artifact.code);
    auto stage = artifact.stage == GraphicsArtifactStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    auto range = module->GetPushConstants();
    auto begin = range.size ? range.offset : artifact.pushConstantOffset;
    auto end = std::max(range.offset + range.size, artifact.pushConstantOffset + artifact.pushConstantSize);
    if (artifact.pushConstantSize) begin = std::min(begin, artifact.pushConstantOffset);
    if (end > begin) constants.push_back({static_cast<VkShaderStageFlags>(stage), begin, end - begin});
    for (const auto& descriptor : artifact.descriptors) {
        auto found = std::ranges::find_if(bindings, [&](const auto& binding) { return binding.binding == descriptor.kind; });
        if (found == bindings.end()) bindings.push_back({descriptor.kind, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, static_cast<VkShaderStageFlags>(stage), nullptr});
        else found->stageFlags |= stage;
        descriptorMask |= 1u << (descriptor.kind - 59);
    }
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage;
    info.module = module->GetHandle();
    info.pName = "main";
    stages.push_back(info);
    modules.push_back(std::move(module));
}

AnyBox::Shader::Runtime::GraphicsProgram::~GraphicsProgram() {
    layout.reset();
    if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
}
