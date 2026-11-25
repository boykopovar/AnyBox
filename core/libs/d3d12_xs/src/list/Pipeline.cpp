#include "List.hpp"
#include "../pipeline/State.hpp"

void AnyBox::Graphics::CommandList::bindPipeline(PipelineState* next) {
    if (pipeline == next) return;
    if (next->GetGraphics() && guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT) throw std::invalid_argument("Graphics pipeline requires a direct command list");
    if (guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT && guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Pipeline binding is not implemented for this command list type");
    if (!next->IsOffline()) host->SetPipelineState(next->GetHost());
    PipelineState::AddRef(next);
    if (pipeline) PipelineState::Release(pipeline);
    pipeline = next;
    cachePipeline();
}

void AnyBox::Graphics::CommandList::cachePipeline() noexcept {
    auto* destination = guest.driverState.data() + 0x544 - 56;
    if (pipeline) {
        const auto& words = pipeline->GetInlineCommands();
        std::memcpy(destination, words.data(), sizeof(words));
    } else std::memset(destination, 0, 32);
}

void AnyBox::Graphics::CommandList::SetPipelineState(CommandList* self, void* object) {
    if (self->IsClosed()) throw std::logic_error("Pipeline binding requires a recording command list");
    auto* selected = PipelineState::From(object, self->device);
    self->flushGuestCommands();
    self->bindPipeline(selected);
}
