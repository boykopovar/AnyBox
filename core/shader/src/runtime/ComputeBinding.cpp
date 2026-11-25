#include <anybox/shader/runtime/ComputeBinding.hpp>
#include "ComputeData.hpp"

using namespace AnyBox::Shader::Runtime;

ComputeBinding::ComputeBinding(const ComputePipeline& pipeline, ComputeMemory& memory) : pipeline(pipeline) {
    if (pipeline.RequiresResources()) throw std::invalid_argument("Compute resource descriptors require an explicit resource set");
    if (&memory.GetContext() != &pipeline.GetContext()) throw std::invalid_argument("Compute pipeline and memory belong to different Vulkan devices");
    if (pipeline.GetShaderDataSize()) data = std::make_unique<ComputeData>(pipeline, memory);
    else if (pipeline.GetDescriptorMask()) descriptors = std::make_unique<ComputeDescriptors>(pipeline, memory);
}

ComputeBinding::~ComputeBinding() = default;

std::span<const VkDescriptorSet> ComputeBinding::GetSets() const noexcept { return data ? data->GetSets() : descriptors ? descriptors->GetSets() : std::span<const VkDescriptorSet>{}; }

void ComputeBinding::Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> externalData) const {
    if (!commands) throw std::invalid_argument("Compute dispatch command buffer is null");
    if (!data && !externalData.empty()) throw std::invalid_argument("Compute dispatch supplies unexpected external shader data");
    std::uint32_t offset = data ? data->Write(externalData) : 0;
    pipeline.GetPipeline().Dispatch(commands, constants, groups, GetSets(), {&offset, data ? 1u : 0u});
}
