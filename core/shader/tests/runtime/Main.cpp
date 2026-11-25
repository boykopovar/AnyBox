#include <anybox/shader/runtime/ComputePipeline.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <anybox/ExceptionTest.hpp>
#include "Commands.hpp"
#include <cstring>

namespace {
constexpr auto Source = R"(#version 450
#extension GL_EXT_buffer_reference : require
layout(local_size_x=1) in;
layout(buffer_reference, std430, buffer_reference_align=4) buffer Data { uint value; };
layout(push_constant) uniform Parameters { Data outputBuffer; uint value; } args;
void main() { args.outputBuffer.value = args.value; }
)";

void Check() {
    using namespace AnyBox;
    Vulkan::Context context(0);
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(std::as_bytes(std::span(Source, std::strlen(Source))));
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 12;
    artifact.code = Vulkan::Tests::CompileCompute(Source);
    auto bytes = Shader::EncodeComputeArtifact(artifact);
    auto loaded = Shader::DecodeComputeArtifact(bytes, artifact.sourceHash);
    Shader::Runtime::ComputePipeline pipeline(context, loaded);
    if (pipeline.GetDescriptorLayout()) throw std::runtime_error("Descriptor-free shader created an unexpected descriptor layout");
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, 4, usage, properties);
    std::memset(output.Map().data(), 0, output.Map().size());
    auto address = output.GetAddress();
    std::array constants{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), 0xABCDEF12u};
    Vulkan::Tests::Commands commands(context);
    auto command = commands.Begin();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    pipeline.GetPipeline().Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1});
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(command);
    std::uint32_t value;
    std::memcpy(&value, output.Map().data(), sizeof(value));
    if (value != constants[2]) throw std::runtime_error("Serialized compute pipeline returned an incorrect GPU result");
    auto reject = [&](auto invalid) {
        try { Shader::Runtime::ComputePipeline selected(context, invalid); }
        catch (const std::exception&) { return; }
        throw std::runtime_error("Serialized compute pipeline accepted unsupported runtime data");
    };
    loaded.executionFlags = 1;
    reject(loaded);
    loaded = artifact;
    loaded.descriptors.push_back({0, {}});
    reject(loaded);
    loaded = artifact;
    loaded.memoryOffsetCount = 1;
    reject(loaded);
}

void CheckLegacyPushConstants() {
    using namespace AnyBox;
    constexpr auto source = R"(#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout(local_size_x=1) in;
layout(buffer_reference, std430, buffer_reference_align=4) buffer Data { uint value; };
layout(push_constant, std430) uniform Parameters { uint words[32]; } args;
void main() {
    uint64_t address = uint64_t(args.words[0]) | (uint64_t(args.words[1]) << 32);
    Data outputBuffer = Data(address);
    outputBuffer.value = args.words[2];
}
)";
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(std::as_bytes(std::span(source, std::strlen(source))));
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 12;
    artifact.pushDataStart = 0;
    artifact.memoryOffset = 3;
    artifact.userRegisters = {0, 1, 2};
    artifact.code = Vulkan::Tests::CompileCompute(source);
    auto encoded = Shader::EncodeComputeArtifact(artifact);
    auto loaded = Shader::DecodeComputeArtifact(encoded, artifact.sourceHash);
    if (loaded != artifact) throw std::runtime_error("Legacy push constant artifact changed after serialization");

    Vulkan::Context context(0);
    Shader::Runtime::ComputePipeline pipeline(context, loaded);
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto output = Vulkan::Buffer::Allocate(context, sizeof(std::uint32_t), usage, properties);
    auto mapped = output.Map();
    std::memset(mapped.data(), 0, mapped.size());
    auto address = output.GetAddress();
    std::array constants{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), 0x9ABCDEF0u};
    Vulkan::Tests::Commands commands(context);
    auto command = commands.Begin();
    std::array<std::byte, 128> padded{};
    bool rejectedPaddedUpload = false;
    try { pipeline.GetPipeline().Dispatch(command, padded, {1, 1, 1}); }
    catch (const std::invalid_argument&) { rejectedPaddedUpload = true; }
    if (!rejectedPaddedUpload) throw std::runtime_error("Legacy push constant pipeline accepted an oversized upload");
    pipeline.GetPipeline().Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1});
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(command);
    std::uint32_t value;
    std::memcpy(&value, mapped.data(), sizeof(value));
    if (value != constants[2]) throw std::runtime_error("Legacy push constant pipeline returned an incorrect GPU result");
}
}

int main() {
    return AnyBox::Tests::Run([] {
        Check();
        CheckLegacyPushConstants();
    });
}
