#include "ResourceFixture.hpp"

void AnyBox::Shader::RuntimeTests::CheckResourceFailures(const Runtime::ComputePipeline& pipeline, std::span<const Runtime::ComputeBufferDescriptors> buffers, std::span<const Runtime::ComputeImageDescriptors> images) {
    using namespace Runtime;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers.first(1), images); });
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers, images.first(1)); });
    auto invalidBuffers = std::vector(buffers.begin(), buffers.end());
    auto invalidImages = std::vector(images.begin(), images.end());
    invalidBuffers[1] = invalidBuffers[0];
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    invalidImages[2] = invalidImages[0];
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers, invalidImages); });
    auto buffer = buffers[0].values[0];
    invalidBuffers.assign(buffers.begin(), buffers.end());
    invalidBuffers[0].values = {&buffer, 1};
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    invalidBuffers[1].values = {&buffer, 1};
    invalidBuffers[0] = buffers[0];
    buffer.buffer = VK_NULL_HANDLE;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    buffer = buffers[1].values[0];
    buffer.range = 0;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    buffer.range = static_cast<VkDeviceSize>(pipeline.GetContext().GetProperties().limits.maxStorageBufferRange) + 1;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    if (pipeline.GetContext().GetProperties().limits.minStorageBufferOffsetAlignment > 1) {
        buffer = buffers[1].values[0];
        buffer.offset = 1;
        Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, invalidBuffers, images); });
    }
    invalidImages.assign(images.begin(), images.end());
    auto image = images[1].values[0];
    invalidImages[1].values = {&image, 1};
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers, invalidImages); });
    image = images[1].values[0];
    image.imageView = VK_NULL_HANDLE;
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers, invalidImages); });
    invalidImages.assign(images.begin(), images.end());
    image = images[2].values[0];
    image.sampler = VK_NULL_HANDLE;
    invalidImages[2].values = {&image, 1};
    Reject<std::invalid_argument>([&] { ComputeResourceSet invalid(pipeline, buffers, invalidImages); });
}
