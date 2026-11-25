#pragma once

#include <anybox/shader/runtime/ComputeResourceSet.hpp>
#include <anybox/vulkan/ImageView.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include "Commands.hpp"
#include <anybox/ExceptionTest.hpp>

namespace AnyBox::Shader::RuntimeTests {
inline void Require(bool condition) {
    if (!condition) throw std::runtime_error("Compute resource binding test failed");
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); }
    catch (const TException&) { return; }
    throw std::runtime_error("Compute resource binding accepted an invalid argument");
}

class Sampler {
public:
    explicit Sampler(Vulkan::Context& context) : context(context) {
        VkSamplerCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        description.maxLod = 0;
        Vulkan::Check(context.GetFunctions().createSampler(context.GetDevice(), &description, nullptr, &handle), "Create compute resource test sampler");
    }
    ~Sampler() { context.GetFunctions().destroySampler(context.GetDevice(), handle, nullptr); }
    VkSampler GetHandle() const noexcept { return handle; }

private:
    Vulkan::Context& context;
    VkSampler handle = VK_NULL_HANDLE;
};

inline ComputeArtifact MakeResourceShader(Vulkan::Context& context, const char* source) {
    ComputeArtifact artifact;
    artifact.threads = {1, 1, 1};
    artifact.hostSubgroupSize = artifact.waveSize = context.GetSubgroupProperties().subgroupSize;
    artifact.code = Vulkan::Tests::CompileCompute(source);
    return artifact;
}

inline void Barrier(Vulkan::Context& context, VkCommandBuffer commands, VkPipelineStageFlags source, VkPipelineStageFlags destination, VkAccessFlags writes, VkAccessFlags reads) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = writes;
    barrier.dstAccessMask = reads;
    context.GetFunctions().pipelineBarrier(commands, source, destination, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void CheckResourceFailures(const Runtime::ComputePipeline& pipeline, std::span<const Runtime::ComputeBufferDescriptors> buffers, std::span<const Runtime::ComputeImageDescriptors> images);
}
