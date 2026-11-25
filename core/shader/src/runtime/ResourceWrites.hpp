#pragma once

#include <anybox/shader/runtime/ComputeResourceSet.hpp>

namespace AnyBox::Shader::Runtime {
std::vector<VkWriteDescriptorSet> BuildComputeResourceWrites(const ComputePipeline& pipeline, std::span<const ComputeBufferDescriptors> buffers, std::span<const ComputeImageDescriptors> images);
}
