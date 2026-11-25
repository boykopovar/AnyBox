#include "ComputeFixture.hpp"
#include "PreparedCache.hpp"
#include "../../src/fence/Fence.hpp"
#include "../../src/host/Com.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/queue/Queue.hpp"
#include "../../src/resource/Resource.hpp"
#include "../../../../graphics/vulkan/tests/Tests.hpp"
#include <d3dcompiler.h>
#include <xmem.hpp>
#include <algorithm>
#include <iostream>

namespace {
using namespace AnyBox;
using namespace AnyBox::Graphics;
using namespace AnyBox::Graphics::Tests;

std::vector<std::byte> CompileInlineDispatchDxbc() {
    constexpr char source[] = R"(RWByteAddressBuffer output : register(u0);
cbuffer Arguments : register(b1) { uint byteOffset; uint value; };
[numthreads(1, 1, 1)] void main() { output.Store(byteOffset, value); }
)";
    ID3DBlob* output = nullptr;
    auto result = D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "main", "cs_5_1", D3DCOMPILE_ENABLE_STRICTNESS, 0, &output, nullptr);
    ComOwner<ID3DBlob> blob(output);
    Require(result == S_OK && blob, "Inline dispatch HLSL fixture compilation failed");
    auto* first = static_cast<const std::byte*>(blob->GetBufferPointer());
    return {first, first + blob->GetBufferSize()};
}

Shader::ComputeArtifact MakeInlineDispatchArtifact(std::span<const std::byte> dxbc) {
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(dxbc);
    artifact.threads = {1, 1, 1};
    artifact.pushConstantSize = 6 * sizeof(std::uint32_t);
    artifact.pushDataStart = 0;
    artifact.memoryOffset = 6;
    artifact.userRegisters = {0, 1, 2, 3, 4, 5};
    artifact.descriptors = {{59, {}}, {60, {}}};
    artifact.buffers = {{Shader::ResourceClass::UnorderedAccess, 0, 0, 0, 1, Shader::BufferKind::Raw},
        {Shader::ResourceClass::ConstantBuffer, 0, 0, 1, 1, Shader::BufferKind::Constant, 0, 16}};
    artifact.code = Vulkan::Tests::CompileCompute(R"(#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout(local_size_x = 1) in;
layout(push_constant, std430) uniform Arguments {
    uvec2 address;
    uvec2 sizeBytes;
    uint offset;
    uint value;
} args;
struct Range { uint64_t begin; uint64_t end; uint64_t deviceAddress; uint permissions; uint reserved; };
layout(set = 0, binding = 59, std430) readonly buffer PageTable { uvec4 header; Range ranges[]; } pages;
layout(set = 0, binding = 60, std430) buffer FaultBuffer { uint words[]; } faults;
layout(buffer_reference, std430, buffer_reference_align = 4) buffer Output { uint value; };
void main() {
    uint64_t offset = uint64_t(args.offset);
    uint64_t length = uint64_t(args.sizeBytes.x) | (uint64_t(args.sizeBytes.y) << 32);
    uint64_t address = uint64_t(args.address.x) | (uint64_t(args.address.y) << 32);
    uint64_t target = address + offset;
    const uint64_t storeBytes = uint64_t(4);
    if (pages.header.x == 1u && pages.header.z == 32u && offset + storeBytes <= length) {
        for (uint i = 0; i < pages.header.y; ++i) {
            Range entry = pages.ranges[i];
            if (entry.begin <= target && target + storeBytes <= entry.end && (entry.permissions & 2u) != 0u) {
                Output(entry.deviceAddress + target - entry.begin).value = args.value;
                return;
            }
        }
    }
    faults.words[1] = 1u;
    faults.words[0] = 2u;
}
)");
    return artifact;
}

void CheckPreparedInlineDispatch(Device* device) {
    const auto& subgroups = device->GetGpuContext().GetSubgroupProperties();
    if (subgroups.subgroupSize != 32 || (subgroups.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) == 0) return;
    std::uint64_t frame = 0;
    std::size_t count = 1;
    Require(XMemAllocatePhysicalPages(0x280, &count, &frame) == 1 && count == 1, "Inline dispatch physical allocation failed");
    auto freePage = [](std::uint64_t* page) { XMemFreePhysicalPages(1, page); };
    std::unique_ptr<std::uint64_t, decltype(freePage)> physical(&frame, freePage);
    auto freeAddress = [](void* address) { VirtualFree(address, 0, MEM_RELEASE); };
    std::unique_ptr<void, decltype(freeAddress)> reservation(XMemVirtualAlloc(nullptr, PhysicalPageSize, 0x20402000, 0x23, 0x2004), freeAddress);
    auto unmapAddress = [](void* address) { XMemMapPhysicalPages(address, 1, nullptr); };
    std::unique_ptr<void, decltype(unmapAddress)> mapping(XMemMapPhysicalPages(reservation.get(), 1, &frame), unmapAddress);
    std::memset(mapping.get(), 0, PhysicalPageSize);
    D3D12_RESOURCE_DESC1 buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = PhysicalPageSize;
    buffer.Height = 1;
    buffer.DepthOrArraySize = 1;
    buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    void* output = nullptr;
    auto createBuffer = Method<decltype(&Device::CreatePlacedResourceLayoutX)>(device, 133);
    Require(createBuffer(device, reinterpret_cast<std::uintptr_t>(mapping.get()), &buffer, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output) == S_OK && output,
        "Inline dispatch placed buffer creation failed");
    auto* placed = static_cast<Resource*>(output);
    std::unique_ptr<Resource, decltype(&Resource::Release)> resource(placed, Method<decltype(&Resource::Release)>(placed, 2));
    constexpr std::uint32_t viewOffset = 64;
    auto address = Method<decltype(&Resource::GetGPUVirtualAddress)>(placed, 11)(placed) + viewOffset;

    ComputeFixture fixture(device);
    auto code = CompileInlineDispatchDxbc();
    PreparedCache cache;
    cache.Save(MakeInlineDispatchArtifact(code));
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot());
    description.CS = {code.data(), code.size()};
    auto createPipeline = Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62);
    Require(createPipeline(device, &description, 0, nullptr, PipelineStateId, &output) == S_OK && output, "Inline dispatch pipeline creation failed");
    auto* state = static_cast<PipelineState*>(output);
    std::unique_ptr<PipelineState, decltype(&PipelineState::Release)> pipeline(state, Method<decltype(&PipelineState::Release)>(state, 2));
    Require(state->GetOffline(), "Inline dispatch fixture requires the prepared backend");

    auto* list = fixture.GetList();
    Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
    auto* begin = ComputeFixture::GetWrite(list);
    constexpr auto pipelineBytes = 8 * sizeof(std::uint32_t);
    std::memcpy(begin, reinterpret_cast<const std::byte*>(state) + 16, pipelineBytes);
    Require(begin[0] == 0x8AB00007, "Unexpected inline compute pipeline record");
    auto constantHeader = ComputeFixture::ReadHeader(list, 3) + (1u << 16);
    constexpr std::uint32_t dispatch = 0x4002C400, firstOffset = 0, secondOffset = 16;
    constexpr std::uint32_t firstValue = 0x12345678, secondValue = 0xABCDEF12;
    std::array records{ComputeFixture::ReadHeader(list, 0), static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32),
        constantHeader, firstOffset, firstValue, dispatch, 1u, 1u, 1u,
        constantHeader, secondOffset, secondValue, dispatch, 1u, 1u, 1u};
    auto* end = std::copy(records.begin(), records.end(), begin + 8);
    *end = dispatch;
    ComputeFixture::SetWrite(list, end + 1);
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    Reject<std::invalid_argument>([&] { close(list); });
    Reject<std::logic_error>([&] { fixture.GetArguments().Read(0, 0, 2); });
    ComputeFixture::SetWrite(list, end);
    begin[9] = begin[10] = 0;
    Reject<std::invalid_argument>([&] { close(list); });
    begin[9] = records[1];
    begin[10] = records[2];
    Require(close(list) == S_OK, "Prepared inline dispatch list did not close");
    pipeline.reset();

    D3D12_COMMAND_QUEUE_DESC queueDescription{D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    Require(Method<decltype(&Device::CreateCommandQueue)>(device, 8)(device, &queueDescription, QueueId, &output) == S_OK && output, "Inline dispatch queue creation failed");
    auto* commandQueue = static_cast<Queue*>(output);
    std::unique_ptr<Queue, decltype(&Queue::Release)> queue(commandQueue, Method<decltype(&Queue::Release)>(commandQueue, 2));
    Require(Method<decltype(&Device::CreateFence)>(device, 36)(device, 0, D3D12_FENCE_FLAG_NONE, FenceId, &output) == S_OK && output, "Inline dispatch fence creation failed");
    auto* publicFence = static_cast<Fence*>(output);
    std::unique_ptr<Fence, decltype(&Fence::Release)> fence(publicFence, Method<decltype(&Fence::Release)>(publicFence, 2));
    Require(Method<decltype(&Queue::Wait)>(commandQueue, 15)(commandQueue, publicFence, 1) == S_OK, "Inline dispatch queue wait failed");
    void* lists[]{list};
    Method<decltype(&Queue::ExecuteCommandLists)>(commandQueue, 10)(commandQueue, 1, lists);
    Require(Method<decltype(&CommandList::Reset)>(list, 10)(list, fixture.GetAllocator(), nullptr) == S_OK, "Submitted prepared list did not reset");
    Require(Method<decltype(&Queue::Signal)>(commandQueue, 14)(commandQueue, publicFence, 2) == S_OK, "Inline dispatch fence signal failed");
    Require(Method<decltype(&Fence::GetCompletedValue)>(publicFence, 8)(publicFence) == 0, "Prepared submission passed an unsatisfied queue wait");
    Require(Method<decltype(&Fence::Signal)>(publicFence, 10)(publicFence, 1) == S_OK, "Inline dispatch CPU fence signal failed");
    std::unique_ptr<void, decltype(&CloseHandle)> event(CreateEventW(nullptr, FALSE, FALSE, nullptr), CloseHandle);
    Require(event != nullptr, "Inline dispatch fence event creation failed");
    Require(Method<decltype(&Fence::SetEventOnCompletion)>(publicFence, 9)(publicFence, 2, event.get()) == S_OK, "Inline dispatch fence event registration failed");
    Require(WaitForSingleObject(event.get(), 10000) == WAIT_OBJECT_0, "Inline dispatch GPU work did not finish");

    void* mapped = nullptr;
    Require(Method<decltype(&Resource::Map)>(placed, 8)(placed, 0, nullptr, &mapped) == S_OK && mapped == mapping.get(), "Inline dispatch buffer mapping failed");
    const auto* bytes = static_cast<const std::byte*>(mapped);
    std::array<std::uint32_t, 2> values{};
    std::memcpy(&values[0], bytes + viewOffset + firstOffset, sizeof(values[0]));
    std::memcpy(&values[1], bytes + viewOffset + secondOffset, sizeof(values[1]));
    Method<decltype(&Resource::Unmap)>(placed, 9)(placed, 0, nullptr);
    Require(values[0] == firstValue && values[1] == secondValue, "Prepared inline dispatch did not write both expected values");
    resource.reset();
    mapping.reset();
    std::cout << "Prepared inline compute wrote both XMem values after the public queue fence\n";
}
}

void AnyBox::Graphics::Tests::CheckComputeInline(Device* device) {
    ComputeFixture fixture(device);
    auto* list = fixture.GetList();
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
    auto* begin = ComputeFixture::GetWrite(list);
    std::array records{ComputeFixture::ReadHeader(list, 0), 0xABC00000u, 2u, ComputeFixture::ReadHeader(list, 3) + (2u << 16) + 8u, 7u, 8u, 9u};
    std::copy(records.begin(), records.end(), begin);
    ComputeFixture::SetWrite(list, begin + records.size());
    Method<decltype(&CommandList::SetComputeRoot32BitConstant)>(list, 37)(list, 3, 33, 3);
    Require(ComputeFixture::GetWrite(list) == begin, "Vtable root update did not consume the preceding inline records");
    Require(fixture.GetArguments().Read(0, 0, 2)[0] == 0xABC00000, "Inline root descriptor was not retained");
    Require(fixture.GetArguments().Read(3, 2, 3)[0] == 7 && fixture.GetArguments().Read(3, 2, 3)[1] == 33 && fixture.GetArguments().Read(3, 2, 3)[2] == 9, "Mixed inline and vtable root updates changed their order");
    begin[0] = ComputeFixture::ReadHeader(list, 3);
    begin[1] = 55;
    begin[2] = 0xDEAD0000;
    ComputeFixture::SetWrite(list, begin + 3);
    Reject<std::runtime_error>([&] { close(list); });
    Reject<std::logic_error>([&] { fixture.GetArguments().Read(3, 0, 1); });
    ComputeFixture::SetWrite(list, begin + 2);
    Require(close(list) == 0, "Supported inline root record could not close");
    Require(fixture.GetArguments().Read(3, 0, 1)[0] == 55, "Closing an inline root stream did not retain its state");
    Method<decltype(&CommandList::Reset)>(list, 10)(list, fixture.GetAllocator(), nullptr);
    Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
    begin = ComputeFixture::GetWrite(list);
    auto header = ComputeFixture::ReadHeader(list, 3);
    for (unsigned index = 0; index < 95; ++index) {
        begin[index * 2] = header;
        begin[index * 2 + 1] = index;
    }
    begin[190] = header + (13u << 16);
    for (unsigned index = 0; index < 14; ++index) begin[191 + index] = 100 + index;
    ComputeFixture::SetWrite(list, begin + 205);
    Require(close(list) == 0 && fixture.GetArguments().Read(3, 0, 14)[13] == 113, "Inline root record crossing the visible buffer boundary was not consumed safely");
    CheckPreparedInlineDispatch(device);
}
