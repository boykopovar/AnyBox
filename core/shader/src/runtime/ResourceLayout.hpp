#pragma once

#include <anybox/shader/runtime/ComputePipeline.hpp>

namespace AnyBox::Shader::Runtime {
std::vector<VkDescriptorSetLayoutBinding> BuildComputeResourceLayout(const Vulkan::Context& context, std::span<const ArtifactDescriptor> descriptors);
}
