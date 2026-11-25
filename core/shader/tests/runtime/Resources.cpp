#include "ResourceFixture.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <cstring>

namespace {
constexpr auto Source = R"(#version 450
layout(local_size_x=1) in;
layout(set=0,binding=0,std430) buffer Output { uint value; } outputs[2];
layout(set=0,binding=61,std430) readonly buffer RootData { uint value; } rootData;
layout(set=0,binding=1) uniform utexture2D inputs[2];
layout(set=0,binding=57) uniform sampler samplers[1];
layout(r32ui,set=0,binding=36) uniform uimage2D resultImages[1];
layout(push_constant) uniform Params { uint userWord; uint packedOffsets; } args;
void main() {
    uint a = texture(usampler2D(inputs[0], samplers[0]), vec2(0.5)).r;
    uint b = texture(usampler2D(inputs[1], samplers[0]), vec2(0.5)).r;
    imageStore(resultImages[0], ivec2(0), uvec4(a + b + rootData.value + args.userWord + (args.packedOffsets & 255u) + ((args.packedOffsets >> 8) & 255u)));
    outputs[0].value = imageLoad(resultImages[0], ivec2(0)).r;
    outputs[1].value = a * b;
}
)";
}

void CheckComputeResourceSets(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader;
    using namespace Shader::Runtime;
    using namespace Shader::RuntimeTests;
    auto artifact = MakeResourceShader(context, Source);
    artifact.descriptors = {{57, {0}}, {0, {0, 1}}, {1, {0, 1}}, {36, {2}}, {61, {}}};
    artifact.pushConstantSize = 8;
    artifact.pushDataStart = 0;
    artifact.userRegisters = {4};
    artifact.memoryOffset = 1;
    artifact.memoryOffsetCount = 2;
    ComputePipeline pipeline(context, artifact);
    Require(pipeline.RequiresResources() && pipeline.GetDescriptorBindings().size() == 5);
    ComputeMemory memory(context, MemoryTable({}));
    Reject<std::invalid_argument>([&] { ComputeBinding invalid(pipeline, memory); });
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, properties);
    auto second = Vulkan::Buffer::Allocate(context, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, properties);
    auto roots = Vulkan::Buffer::Allocate(context, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, properties);
    std::uint32_t rootValue = 7;
    std::memcpy(roots.Map().data(), &rootValue, 4);
    Vulkan::ImageDescription description;
    description.format = VK_FORMAT_R32_UINT;
    description.extent = {1, 1, 1};
    description.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    Vulkan::Image input(context, description);
    Vulkan::Image result(context, description);
    Vulkan::ImageView inputView(input, {});
    Vulkan::ImageView resultView(result, {});
    Sampler sampler(context);
    std::array<VkDescriptorBufferInfo, 2> outputs{{{output.GetHandle(), 0, 4}, {second.GetHandle(), 0, 4}}};
    VkDescriptorBufferInfo root{roots.GetHandle(), 0, 4};
    std::array<ComputeBufferDescriptors, 2> buffers{{{0, outputs}, {61, {&root, 1}}}};
    std::array<VkDescriptorImageInfo, 2> sampled{{{VK_NULL_HANDLE, inputView.GetHandle(), VK_IMAGE_LAYOUT_GENERAL}, {VK_NULL_HANDLE, inputView.GetHandle(), VK_IMAGE_LAYOUT_GENERAL}}};
    VkDescriptorImageInfo storage{VK_NULL_HANDLE, resultView.GetHandle(), VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo sampling{sampler.GetHandle(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    std::array<ComputeImageDescriptors, 3> images{{{1, sampled}, {36, {&storage, 1}}, {57, {&sampling, 1}}}};
    ComputeResourceSet resources(pipeline, buffers, images);
    CheckResourceFailures(pipeline, buffers, images);
    outputs[0].buffer = VK_NULL_HANDLE;
    sampled[0].imageView = VK_NULL_HANDLE;

    for (std::uint32_t iteration = 0; iteration < 2; ++iteration) {
        Vulkan::Tests::Commands commands(context);
        auto command = commands.Begin();
        std::array<VkImageMemoryBarrier, 2> transitions{};
        for (std::size_t index = 0; index < transitions.size(); ++index) {
            auto& transition = transitions[index];
            transition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            transition.oldLayout = iteration == 0 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
            transition.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            transition.srcQueueFamilyIndex = transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition.srcAccessMask = iteration == 0 ? 0 : VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            transition.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            transition.image = index == 0 ? input.GetHandle() : result.GetHandle();
            transition.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        }
        context.GetFunctions().pipelineBarrier(command, iteration == 0 ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, transitions.data());
        VkClearColorValue color{};
        color.uint32[0] = 5 + iteration;
        context.GetFunctions().clearColorImage(command, input.GetHandle(), VK_IMAGE_LAYOUT_GENERAL, &color, 1, &transitions[0].subresourceRange);
        Barrier(context, command, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
        std::array<std::uint32_t, 2> constants{11, 0x0302};
        pipeline.GetPipeline().Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1}, resources.GetSets());
        Barrier(context, command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT);
        commands.Submit(command);
        std::uint32_t sum = 0;
        std::uint32_t product = 0;
        std::memcpy(&sum, output.Map().data(), 4);
        std::memcpy(&product, second.Map().data(), 4);
        Require(sum == (5 + iteration) * 2 + rootValue + 16 && product == (5 + iteration) * (5 + iteration));
    }
}
