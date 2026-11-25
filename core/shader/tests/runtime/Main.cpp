#include <anybox/shader/runtime/ComputePipeline.hpp>
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <anybox/ExceptionTest.hpp>
#include "Commands.hpp"
#include <cstring>

void CheckMemoryTable();
void CheckGraphicsProgram(AnyBox::Vulkan::Context& context);
void CheckComputeBindings(AnyBox::Vulkan::Context& context);
void CheckExternalShaderData(AnyBox::Vulkan::Context& context);
void CheckComputeResourceSets(AnyBox::Vulkan::Context& context);
void CheckComputeResourceLimits(AnyBox::Vulkan::Context& context);
void CheckExtendedImageGather(AnyBox::Vulkan::Context& context);

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
    CheckMemoryTable();
    Vulkan::Context context(0);
    CheckComputeResourceSets(context);
    CheckExtendedImageGather(context);
    CheckComputeResourceLimits(context);
    CheckGraphicsProgram(context);
    CheckComputeBindings(context);
    CheckExternalShaderData(context);
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(std::as_bytes(std::span(Source, std::strlen(Source))));
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 12;
    artifact.code = Vulkan::Tests::CompileCompute(Source);
    auto bytes = Shader::EncodeComputeArtifact(artifact);
    auto loaded = Shader::DecodeComputeArtifact(bytes, artifact.sourceHash);
    Shader::Runtime::ComputePipeline pipeline(context, loaded);
    if (pipeline.GetDescriptorLayout()) throw std::runtime_error("Descriptor-free shader created an unexpected descriptor layout");
    Shader::Runtime::ComputeMemory memory(context, Shader::Runtime::MemoryTable({}));
    Shader::Runtime::ComputeBinding binding(pipeline, memory);
    if (!binding.GetSets().empty()) throw std::runtime_error("Descriptor-free shader allocated a compute metadata set");
    {
        Vulkan::Context other(0);
        Shader::Runtime::ComputeMemory foreign(other, Shader::Runtime::MemoryTable({}));
        bool caught = false;
        try { Shader::Runtime::ComputeBinding invalid(pipeline, foreign); }
        catch (const std::invalid_argument&) { caught = true; }
        if (!caught) throw std::runtime_error("Compute binding accepted metadata from a different Vulkan device");
    }
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
    memory.Begin(command);
    binding.Dispatch(command, std::as_bytes(std::span(constants)), {1, 1, 1});
    memory.End(command);
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(command);
    memory.CheckFault();
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
    loaded = artifact;
    loaded.hostSubgroupSize = 64;
    reject(loaded);
    loaded = artifact;
    loaded.waveSize = 64;
    reject(loaded);
    loaded = artifact;
    loaded.threads[0] = 2;
    reject(loaded);
}
}

int main() { return AnyBox::Tests::Run(Check); }
