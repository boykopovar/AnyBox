#include <anybox/shader/runtime/ComputePipeline.hpp>
#include "Requirements.hpp"
#include "ResourceLayout.hpp"
#include <vector>

using namespace AnyBox::Shader::Runtime;

ComputePipeline::ComputePipeline(AnyBox::Vulkan::Context& context, const ComputeArtifact& artifact) : context(context) {
    CheckRequirements(context, artifact);
    resources = artifact.memoryOffsetCount != 0;
    bindings = BuildComputeResourceLayout(context, artifact.descriptors);
    for (const auto& descriptor : artifact.descriptors) {
        if (descriptor.kind == MemoryTableBinding || descriptor.kind == FaultBufferBinding || descriptor.kind == ShaderDataBinding) {
            if (descriptor.kind == ShaderDataBinding) shaderDataSize = (artifact.memoryOffset + (artifact.memoryOffsetCount + 3) / 4) * 4;
            descriptorMask |= 1u << (descriptor.kind - MemoryTableBinding);
        } else resources = true;
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
        pipeline = std::make_unique<Vulkan::ComputePipeline>(context, artifact.code, artifact.pushConstantSize, "main", layouts, shaderDataSize ? 1u : 0u);
    } catch (...) {
        if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
        throw;
    }
}

ComputePipeline::~ComputePipeline() {
    pipeline.reset();
    if (descriptorLayout) context.GetFunctions().destroyDescriptorSetLayout(context.GetDevice(), descriptorLayout, nullptr);
}
