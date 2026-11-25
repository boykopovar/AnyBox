#include "Tests.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/list/List.hpp"

void CheckPreparedGraphicsInitial(AnyBox::Graphics::Device* device, AnyBox::Graphics::PipelineState* pipeline) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    void* output = nullptr;
    auto createAllocator = Method<decltype(&Device::CreateCommandAllocator)>(device, 9);
    auto createList = Method<decltype(&Device::CreateCommandList)>(device, 12);
    createAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT, AllocatorId, &output);
    auto* allocator = static_cast<Allocator*>(output);
    Require(createList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, pipeline, XboxGraphicsListId, &output) == S_OK && output, "Direct command list creation with a graphics pipeline failed");
    auto* list = static_cast<CommandList*>(output);
    Require(list->GetPipelineState() == pipeline, "Direct command list lost its initial graphics pipeline");
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    auto reset = Method<decltype(&CommandList::Reset)>(list, 10);
    Require(close(list) == S_OK && reset(list, allocator, pipeline) == S_OK, "Direct command list reset with a graphics pipeline failed");
    Require(close(list) == S_OK && reset(list, allocator, nullptr) == S_OK, "Direct command list could not clear its graphics pipeline");
    Method<decltype(&CommandList::SetPipelineState)>(list, 25)(list, pipeline);
    Require(list->GetPipelineState() == pipeline, "Direct command list could not bind a graphics pipeline");
    Require(close(list) == S_OK, "Direct graphics command list close failed");
    createAllocator(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, AllocatorId, &output);
    auto* computeAllocator = static_cast<Allocator*>(output);
    Reject<std::invalid_argument>([&] { createList(device, 0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator, pipeline, XboxGraphicsListId, &output); });
    Require(!output, "Compute command list creation accepted an initial graphics pipeline");
    Require(Method<decltype(&Allocator::Release)>(computeAllocator, 2)(computeAllocator) == 0, "Rejected graphics command list creation retained its allocator");
    Require(Method<decltype(&CommandList::Release)>(list, 2)(list) == 0, "Direct graphics command list retained its final reference");
    Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Direct graphics allocator retained its final reference");
}
