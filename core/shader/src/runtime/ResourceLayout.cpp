#include "ResourceLayout.hpp"
#include <algorithm>

namespace {
VkDescriptorType Type(std::uint32_t kind) {
    if (kind == 0 || kind == 59 || kind == 60 || kind == 61) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    if (kind >= 1 && kind < 29) return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    if (kind >= 29 && kind < 57) return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    if (kind == 57) return VK_DESCRIPTOR_TYPE_SAMPLER;
    if (kind == 62) return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    throw std::runtime_error("Offline compute pipeline descriptor kind is not implemented");
}
}

std::vector<VkDescriptorSetLayoutBinding> AnyBox::Shader::Runtime::BuildComputeResourceLayout(const Vulkan::Context& context, std::span<const ArtifactDescriptor> descriptors) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::array<std::uint64_t, 5> counts{};
    for (const auto& descriptor : descriptors) {
        auto type = Type(descriptor.kind);
        auto array = descriptor.kind < 58;
        if (array == descriptor.resources.empty()) throw std::invalid_argument("Compute descriptor resource indices disagree with its binding kind");
        auto size = array ? descriptor.resources.size() : 1;
        if (size > UINT32_MAX) throw std::length_error("Compute descriptor array exceeds its count field");
        bindings.push_back({descriptor.kind, type, static_cast<std::uint32_t>(size), VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
        auto index = type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? 0 : type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ? 1 : type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ? 2 : type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ? 3 : 4;
        counts[index] += size;
    }
    const auto& limits = context.GetProperties().limits;
    auto buffers = counts[0] + counts[1];
    auto total = buffers + counts[2] + counts[3];
    if (buffers > limits.maxPerStageDescriptorStorageBuffers || buffers > limits.maxDescriptorSetStorageBuffers || counts[1] > limits.maxDescriptorSetStorageBuffersDynamic) throw std::runtime_error("Compute buffer descriptor layout exceeds the device limits");
    if (counts[2] > limits.maxPerStageDescriptorSampledImages || counts[2] > limits.maxDescriptorSetSampledImages) throw std::runtime_error("Compute sampled image descriptor layout exceeds the device limits");
    if (counts[3] > limits.maxPerStageDescriptorStorageImages || counts[3] > limits.maxDescriptorSetStorageImages) throw std::runtime_error("Compute storage image descriptor layout exceeds the device limits");
    if (counts[4] > limits.maxPerStageDescriptorSamplers || counts[4] > limits.maxDescriptorSetSamplers || total > limits.maxPerStageResources) throw std::runtime_error("Compute descriptor layout exceeds the device limits");
    std::ranges::sort(bindings, {}, &VkDescriptorSetLayoutBinding::binding);
    return bindings;
}
