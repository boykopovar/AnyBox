#include "ResourceWrites.hpp"
#include <algorithm>

using namespace AnyBox::Shader::Runtime;

ComputeResourceSet::ComputeResourceSet(const ComputePipeline& pipeline, std::span<const ComputeBufferDescriptors> buffers, std::span<const ComputeImageDescriptors> images) : context(pipeline.GetContext()) {
    auto writes = BuildComputeResourceWrites(pipeline, buffers, images);
    if (writes.empty()) return;
    std::vector<VkDescriptorPoolSize> sizes;
    for (const auto& write : writes) {
        auto found = std::ranges::find(sizes, write.descriptorType, &VkDescriptorPoolSize::type);
        if (found == sizes.end()) sizes.push_back({write.descriptorType, write.descriptorCount});
        else found->descriptorCount += write.descriptorCount;
    }
    const auto& functions = context.GetFunctions();
    try {
        VkDescriptorPoolCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        description.maxSets = 1;
        description.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        description.pPoolSizes = sizes.data();
        Vulkan::Check(functions.createDescriptorPool(context.GetDevice(), &description, nullptr, &pool), "Create compute resource descriptor pool");
        auto layout = pipeline.GetDescriptorLayout();
        VkDescriptorSetAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocation.descriptorPool = pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &layout;
        Vulkan::Check(functions.allocateDescriptorSets(context.GetDevice(), &allocation, &set), "Allocate compute resource descriptor set");
        for (auto& write : writes) write.dstSet = set;
        functions.updateDescriptorSets(context.GetDevice(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    } catch (...) {
        if (pool) functions.destroyDescriptorPool(context.GetDevice(), pool, nullptr);
        throw;
    }
}

ComputeResourceSet::~ComputeResourceSet() { if (pool) context.GetFunctions().destroyDescriptorPool(context.GetDevice(), pool, nullptr); }
