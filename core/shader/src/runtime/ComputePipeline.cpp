#include <anybox/shader/runtime/ComputePipeline.hpp>
#include "Requirements.hpp"
#include <vector>

using namespace AnyBox::Shader::Runtime;

ComputePipeline::ComputePipeline(AnyBox::Vulkan::Context& context, const ComputeArtifact& artifact) : context(context) {
    auto requiredSubgroupSize = CheckRequirements(context, artifact);
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& descriptor : artifact.descriptors) {
        if (descriptor.kind != 59 && descriptor.kind != 60) throw std::runtime_error("Offline compute pipeline descriptor kind is not implemented");
        if (!descriptor.resources.empty()) throw std::runtime_error("Offline compute pipeline BDA descriptor contains unexpected resource indices");
        bindings.push_back({descriptor.kind, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
    }
    try {
        std::vector<VkDescriptorSetLayout> layouts;
        if (!bindings.empty()) {
            VkDescriptorSetLayoutCreateInfo description{};
            description.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            description.bindingCount = static_cast<std::uint32_t>(bindings.size());
            description.pBindings = bindings.data();
            Vulkan::Check(context.GetFunctions().createDescriptorSetLayout(context.GetDevice(), &description, nullptr, &descriptorLayout), "Create offline compute descriptor layout");
            layouts.push_back(descriptorLayout);
        }
        pipeline = std::make_unique<Vulkan::ComputePipeline>(context, artifact.code, artifact.pushConstantSize, "main", layouts, requiredSubgroupSize);
    } catch (...) {
        if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
        throw;
    }
}

ComputePipeline::~ComputePipeline() {
    pipeline.reset();
    if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
}
