#include "ResourceFixture.hpp"

void CheckComputeResourceLimits(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Shader;
    using namespace Runtime;
    using namespace RuntimeTests;
    auto artifact = MakeResourceShader(context, "#version 450\nlayout(local_size_x=1) in; void main() {}\n");
    auto reject = [&](std::vector<ArtifactDescriptor> descriptors) {
        auto invalid = artifact;
        invalid.descriptors = std::move(descriptors);
        Reject<std::exception>([&] { ComputePipeline pipeline(context, invalid); });
    };
    for (auto kind : {58u, 63u, 64u, 256u, 257u}) reject({{kind, {}}});
    for (auto kind : {0u, 1u, 29u, 57u}) reject({{kind, {}}});
    for (auto kind : {59u, 60u, 61u, 62u}) reject({{kind, {0}}});
    reject({{0, {0}}, {0, {1}}});
    const auto& limits = context.GetProperties().limits;
    for (auto [kind, count] : {std::pair{0u, limits.maxPerStageDescriptorStorageBuffers}, {1u, limits.maxPerStageDescriptorSampledImages}, {29u, limits.maxPerStageDescriptorStorageImages}, {57u, limits.maxPerStageDescriptorSamplers}}) {
        std::vector<std::uint32_t> resources(static_cast<std::size_t>(count) + 1);
        reject({{kind, std::move(resources)}});
    }
    ComputePipeline pipeline(context, artifact);
    ComputeResourceSet empty(pipeline, {}, {});
    Require(empty.GetSets().empty());
    VkDescriptorBufferInfo unexpected{};
    ComputeBufferDescriptors buffer{0, {&unexpected, 1}};
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, {&buffer, 1}, {}); });
}
