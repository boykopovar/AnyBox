#include "Descriptors.hpp"
#include "ComputeCompiler.hpp"
#include "Commands.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"
#include <anybox/vulkan/ComputePipeline.hpp>
#include <BdaAbi.hpp>
#include <cstring>
#include <vector>

namespace BufferTests {
void RunGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::Dxil::ComputeCompilation& shader, std::span<const std::uint32_t> expected, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> inputData, std::span<const std::uint32_t> userData, std::span<const std::byte> outputData) {
    namespace Abi = ShaderRecompiler::BdaAbi;
    using namespace AnyBox::Vulkan;
    if (!context.SupportsByteStorage()) throw std::runtime_error("DXIL BDA test requires Vulkan byte storage support");
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr auto properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto input = Buffer::Allocate(context, 64, usage, properties);
    auto output = Buffer::Allocate(context, expected.size_bytes(), usage, properties);
    auto faults = Buffer::Allocate(context, Abi::FaultBufferBytes, usage, properties);
    struct PageTable {
        Abi::Header header;
        std::array<Abi::Range, 2> ranges;
    };
    static_assert(sizeof(PageTable) == 80);
    auto table = Buffer::Allocate(context, sizeof(PageTable), usage, properties);
    auto outputPermissions = Abi::Write | (outputData.empty() ? 0 : Abi::Read);
    PageTable pages{{Abi::Version, 2, sizeof(Abi::Range), 0}, {{{0x100000000, 0x100000040, input.GetAddress(), Abi::Read, 0}, {0x200000000, 0x200000000 + output.GetSize(), output.GetAddress(), outputPermissions, 0}}}};
    for (std::size_t index = 0; index < input.Map().size(); ++index) input.Map()[index] = static_cast<std::byte>((index * 3 + 7) & 255);
    if (!inputData.empty()) {
        if (inputData.size() != input.GetSize()) throw std::runtime_error("DXIL GPU test input data has an invalid size");
        std::memcpy(input.Map().data(), inputData.data(), inputData.size());
    }
    std::memset(output.Map().data(), 255, output.Map().size());
    if (!outputData.empty()) {
        if (outputData.size() != output.GetSize()) throw std::runtime_error("DXIL GPU test output data has an invalid size");
        std::memcpy(output.Map().data(), outputData.data(), outputData.size());
    }
    std::memset(faults.Map().data(), 0, faults.Map().size());
    std::memcpy(table.Map().data(), &pages, sizeof(pages));
    Descriptors descriptors(context, table, faults);
    std::array layouts{descriptors.GetLayout()};
    std::array sets{descriptors.GetSet()};
    ComputePipeline pipeline(context, shader.code, shader.bindings.pushConstantSizeBytes, "main", layouts, shader.hostSubgroupSize);
    AnyBox::Vulkan::Tests::Commands commands(context);
    auto buffer = commands.Begin();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    context.GetFunctions().pipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    std::vector<std::byte> constants(shader.bindings.pushConstantSizeBytes);
    const auto& registers = shader.bindings.layout.userDataRegisters;
    if (!registers.empty() && !shader.bindings.layout.UsesPushData()) throw std::runtime_error("GPU argument test requires inline shader data");
    for (std::size_t index = 0; index < registers.size(); ++index) {
        if (registers[index] >= userData.size() || (index + 1) * 4 > constants.size()) throw std::runtime_error("GPU test user data is incomplete");
        std::memcpy(constants.data() + index * 4, &userData[registers[index]], 4);
    }
    pipeline.Dispatch(buffer, constants, groups, sets);
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    context.GetFunctions().pipelineBarrier(buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
    Abi::Fault fault{};
    std::memcpy(&fault, faults.Map().data(), sizeof(fault));
    if (fault.state != Abi::FaultState::Empty) throw std::runtime_error("DXIL BDA GPU test recorded fault " + std::to_string(static_cast<unsigned>(fault.reason)));
    for (std::size_t index = 0; index < expected.size(); ++index) {
        std::uint32_t value = 0;
        std::memcpy(&value, output.Map().data() + index * sizeof(value), sizeof(value));
        if (value != expected[index]) throw std::runtime_error("DXIL GPU result mismatch at " + std::to_string(index) + ": " + std::to_string(value) + " != " + std::to_string(expected[index]));
    }
}

void CheckGpu(AnyBox::Vulkan::Context& context, const AnyBox::Shader::Dxil::ComputeCompilation& shader, std::uint32_t expected) {
    RunGpu(context, shader, std::span(&expected, 1), {1, 1, 1});
}
}
