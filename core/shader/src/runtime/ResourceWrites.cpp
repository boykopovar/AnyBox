#include "ResourceWrites.hpp"
#include <algorithm>

namespace {
template<class TDescriptor>
const TDescriptor& Find(std::span<const TDescriptor> descriptors, std::uint32_t binding, std::uint32_t count) {
    auto found = std::ranges::find(descriptors, binding, &TDescriptor::binding);
    if (found == descriptors.end() || found->values.size() != count) throw std::invalid_argument("Compute resource descriptor array is missing or has an incorrect length");
    if (std::ranges::count(descriptors, binding, &TDescriptor::binding) != 1) throw std::invalid_argument("Compute resource descriptor binding is repeated");
    return *found;
}
}

std::vector<VkWriteDescriptorSet> AnyBox::Shader::Runtime::BuildComputeResourceWrites(const ComputePipeline& pipeline, std::span<const ComputeBufferDescriptors> buffers, std::span<const ComputeImageDescriptors> images) {
    auto bindings = pipeline.GetDescriptorBindings();
    if (buffers.size() + images.size() != bindings.size()) throw std::invalid_argument("Compute resource descriptor binding count differs from its pipeline");
    const auto& limits = pipeline.GetContext().GetProperties().limits;
    std::vector<VkWriteDescriptorSet> writes;
    for (const auto& binding : bindings) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = binding.binding;
        write.descriptorType = binding.descriptorType;
        write.descriptorCount = binding.descriptorCount;
        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            const auto& entry = Find(buffers, binding.binding, binding.descriptorCount);
            for (const auto& value : entry.values) {
                if (!value.buffer || value.offset % limits.minStorageBufferOffsetAlignment != 0 || value.range == 0 || value.range > limits.maxStorageBufferRange || value.offset > UINT64_MAX - value.range) throw std::invalid_argument("Compute buffer descriptor handle, alignment or range is invalid");
                if (binding.binding == ComputePipeline::ShaderDataBinding && value.range != pipeline.GetShaderDataSize()) throw std::invalid_argument("Compute shader data descriptor range differs from its pipeline");
            }
            write.pBufferInfo = entry.values.data();
        } else {
            const auto& entry = Find(images, binding.binding, binding.descriptorCount);
            for (const auto& value : entry.values) {
                if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                    if (!value.sampler) throw std::invalid_argument("Compute sampler descriptor handle is null");
                } else {
                    if (!value.imageView) throw std::invalid_argument("Compute image descriptor view is null");
                    auto valid = value.imageLayout == VK_IMAGE_LAYOUT_GENERAL || (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE && value.imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    if (!valid) throw std::invalid_argument("Compute image descriptor layout is incompatible with its binding");
                }
            }
            write.pImageInfo = entry.values.data();
        }
        writes.push_back(write);
    }
    return writes;
}
