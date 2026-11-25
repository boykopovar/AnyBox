#include "List.hpp"
#include "Compute.hpp"
#include "Gpu.hpp"
#include "../pipeline/State.hpp"
#include <stdexcept>
#include <memory>

HRESULT AnyBox::Graphics::CommandList::Close(CommandList* self) {
    if (self->IsClosed()) throw std::logic_error("Command list is already closed");
    self->flushGuestCommands();
    if (self->gpu) self->gpu->Close();
    auto result = self->host->Close();
    CheckGraphicsResult(result, "ID3D12GraphicsCommandList::Close");
    self->state |= closedBit;
    return result;
}

HRESULT AnyBox::Graphics::CommandList::Reset(CommandList* self, void* allocator, void* initialState) {
    if (!self->IsClosed()) throw std::logic_error("Recording command list cannot be reset");
    auto* initial = initialState ? PipelineState::From(initialState, self->device) : nullptr;
    if (initial && initial->GetGraphics() && self->guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT) throw std::invalid_argument("Graphics pipeline requires a direct command list");
    if (initial && self->guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT && self->guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Initial pipeline is not implemented for this command list type");
    auto* next = Allocator::From(allocator);
    if (next->GetParent() != self->device) throw std::invalid_argument("Command list and allocator devices differ");
    if (next->GetType() != self->guest.type) throw std::invalid_argument("Command list and allocator types differ");
    Allocator::AddRef(next);
    std::unique_ptr<Allocator, decltype(&Allocator::Release)> heldAllocator(next, &Allocator::Release);
    if (initial) PipelineState::AddRef(initial);
    std::unique_ptr<PipelineState, decltype(&PipelineState::Release)> heldPipeline(initial, &PipelineState::Release);
    auto* nativePipeline = initial && !initial->IsOffline() ? initial->GetHost() : nullptr;
    auto result = self->host->Reset(next->GetHost(), nativePipeline);
    CheckGraphicsResult(result, "ID3D12GraphicsCommandList::Reset");

    delete self->gpu;
    self->gpu = nullptr;
    Allocator::Release(self->allocator);
    self->allocator = heldAllocator.release();
    self->state &= ~closedBit;
    delete self->compute;
    self->compute = nullptr;
    if (self->pipeline) PipelineState::Release(self->pipeline);
    self->pipeline = heldPipeline.release();
    self->resetGuestCommands();
    return result;
}
