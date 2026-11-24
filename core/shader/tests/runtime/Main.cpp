#include <anybox/shader/runtime/ComputePipeline.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <anybox/ExceptionTest.hpp>
#include "Commands.hpp"
#include <cstring>
#include <iostream>

namespace {
constexpr auto Source = R"(#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_KHR_shader_subgroup_basic : require
layout(local_size_x=1) in;
layout(buffer_reference, std430, buffer_reference_align=4) buffer Data { uint value; uint waveSize; };
layout(push_constant) uniform Parameters { Data outputBuffer; uint value; } args;
void main() { args.outputBuffer.value = args.value; args.outputBuffer.waveSize = gl_SubgroupSize; }
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
    auto output = Vulkan::Buffer::Allocate(context, 2 * sizeof(std::uint32_t), usage, properties);
    auto mapped = output.Map();
    auto address = output.GetAddress();
    std::array constants{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), 0xABCDEF12u};
    auto defaultSize = context.GetSubgroupProperties().subgroupSize;
    auto checkDispatch = [&](const Vulkan::ComputePipeline& selected, std::uint32_t waveSize) {
        std::memset(mapped.data(), 0, mapped.size());
        Vulkan::Tests::Commands commands(context);
        auto command = commands.Begin();
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        selected.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1});
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        commands.Submit(command);
        std::array<std::uint32_t, 2> result{};
        std::memcpy(result.data(), mapped.data(), sizeof(result));
        if (result[0] != constants[2] || result[1] != waveSize) throw std::runtime_error("Compute pipeline returned an incorrect GPU result or subgroup size");
        std::cout << context.GetProperties().deviceName << " executed a " << result[1] << "-lane subgroup; reported default " << defaultSize << '\n';
    };
    checkDispatch(pipeline.GetPipeline(), loaded.waveSize);
    for (auto size : {8U, 16U, 32U, 64U}) {
        if (size == defaultSize || !context.CanRequireComputeSubgroupSize(size)) continue;
        Vulkan::ComputePipeline alternate(context, loaded.code, loaded.pushConstantSize, "main", {}, size);
        checkDispatch(alternate, size);
        break;
    }
    if (context.CanRequireComputeSubgroupSize(3) || context.CanRequireComputeSubgroupSize(loaded.waveSize, UINT64_MAX)) throw std::runtime_error("Invalid Vulkan compute subgroup requirement was accepted");
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
}

int main() { return AnyBox::Tests::Run(Check); }
