#include "ResourceFixture.hpp"
#include <spirv/unified1/spirv.hpp>
#include <cstring>

namespace {
constexpr auto Source = R"(#version 450
layout(local_size_x=1) in;
layout(set=0,binding=0,std430) buffer Output { uint value; } result;
layout(set=0,binding=1) uniform utexture2D inputImage;
layout(set=0,binding=57) uniform sampler inputSampler;
void main() {
    const ivec2 offsets[4] = ivec2[4](ivec2(0,0), ivec2(1,0), ivec2(0,1), ivec2(1,1));
    uvec4 values = textureGatherOffsets(usampler2D(inputImage, inputSampler), vec2(0.5), offsets, 0);
    result.value = values.x + values.y + values.z + values.w;
}
)";
}

void CheckExtendedImageGather(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader::Runtime;
    using namespace Shader::RuntimeTests;
    auto artifact = MakeResourceShader(context, Source);
    artifact.descriptors = {{0, {0}}, {1, {0}}, {57, {0}}};
    bool capability = false;
    for (std::size_t offset = 5; offset < artifact.code.size(); offset += artifact.code[offset] >> 16) {
        if ((artifact.code[offset] & 65535) == spv::OpCapability && artifact.code[offset + 1] == spv::CapabilityImageGatherExtended) capability = true;
    }
    Require(capability);
    if (!context.GetEnabledFeatures().shaderImageGatherExtended) {
        Reject<std::runtime_error>([&] { ComputePipeline rejected(context, artifact); });
        return;
    }
    ComputePipeline pipeline(context, artifact);
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, properties);
    Vulkan::ImageDescription description;
    description.format = VK_FORMAT_R32_UINT;
    description.extent = {1, 1, 1};
    description.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    Vulkan::Image image(context, description);
    Vulkan::ImageView view(image, {});
    Sampler sampler(context);
    VkDescriptorBufferInfo buffer{output.GetHandle(), 0, 4};
    ComputeBufferDescriptors buffers{0, {&buffer, 1}};
    VkDescriptorImageInfo sampled{VK_NULL_HANDLE, view.GetHandle(), VK_IMAGE_LAYOUT_GENERAL};
    VkDescriptorImageInfo sampling{sampler.GetHandle(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    std::array<ComputeImageDescriptors, 2> images{{{1, {&sampled, 1}}, {57, {&sampling, 1}}}};
    ComputeResourceSet resources(pipeline, {&buffers, 1}, images);
    Vulkan::Tests::Commands commands(context);
    auto command = commands.Begin();
    VkImageMemoryBarrier transition{};
    transition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transition.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transition.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    transition.srcQueueFamilyIndex = transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transition.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transition.image = image.GetHandle();
    transition.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transition);
    VkClearColorValue color{};
    color.uint32[0] = 17;
    context.GetFunctions().clearColorImage(command, image.GetHandle(), VK_IMAGE_LAYOUT_GENERAL, &color, 1, &transition.subresourceRange);
    Barrier(context, command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    pipeline.GetPipeline().Dispatch(command, {}, {1, 1, 1}, resources.GetSets());
    Barrier(context, command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT);
    commands.Submit(command);
    std::uint32_t value = 0;
    std::memcpy(&value, output.Map().data(), 4);
    Require(value == 68);
}
