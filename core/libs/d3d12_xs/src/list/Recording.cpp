#include "List.hpp"
#include "Compute.hpp"
#include <stdexcept>

HRESULT AnyBox::Graphics::CommandList::Close(CommandList* self) {
    if (self->IsClosed()) throw std::logic_error("Command list is already closed");
    self->flushGuestCommands();
    auto result = self->host->Close();
    CheckGraphicsResult(result, "ID3D12GraphicsCommandList::Close");
    self->state |= closedBit;
    return result;
}

HRESULT AnyBox::Graphics::CommandList::Reset(CommandList* self, void* allocator, void* initialState) {
    if (!self->IsClosed()) throw std::logic_error("Recording command list cannot be reset");
    if (initialState) throw std::runtime_error("Initial command list pipeline state is not implemented");
    auto* next = Allocator::From(allocator);
    if (next->GetParent() != self->device) throw std::invalid_argument("Command list and allocator devices differ");
    if (next->GetType() != self->guest.type) throw std::invalid_argument("Command list and allocator types differ");
    auto result = self->host->Reset(next->GetHost(), nullptr);
    CheckGraphicsResult(result, "ID3D12GraphicsCommandList::Reset");

    Allocator::AddRef(next);
    Allocator::Release(self->allocator);
    self->allocator = next;
    self->state &= ~closedBit;
    delete self->compute;
    self->compute = nullptr;
    delete self->recording;
    self->recording = nullptr;
    self->nativeCommandsRecorded = false;
    self->resetGuestCommands();
    return result;
}
