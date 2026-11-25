#include "Device.hpp"
#include "../list/List.hpp"
#include "../pipeline/State.hpp"
#include <memory>
#include <stdexcept>

namespace {
struct ListDeleter {
    void operator()(ID3D12GraphicsCommandList* value) const noexcept { value->Release(); }
};
HRESULT CreateList(AnyBox::Graphics::Device* self, std::uint32_t nodeMask, D3D12_COMMAND_LIST_TYPE type, void* allocator, void* initialState, const AnyBox::Guid& interfaceId, void** list, std::uint32_t flags) {
    using namespace AnyBox::Graphics;
    if (!list) throw std::invalid_argument("Command list output is null");
    *list = nullptr;
    if (!CommandList::SupportsCreation(type, interfaceId)) throw std::invalid_argument("Unsupported command list creation interface");
    auto* initial = initialState ? PipelineState::From(initialState, self) : nullptr;
    if (initial && initial->GetGraphics() && type != D3D12_COMMAND_LIST_TYPE_DIRECT) throw std::invalid_argument("Graphics pipeline requires a direct command list");
    if (initial && type != D3D12_COMMAND_LIST_TYPE_DIRECT && type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Initial pipeline is not implemented for this command list type");
    auto* allocation = Allocator::From(allocator);
    if (allocation->GetParent() != self) throw std::invalid_argument("Command list and allocator devices differ");
    if (allocation->GetType() != type) throw std::invalid_argument("Command list and allocator types differ");
    if (interfaceId == DmaListId && static_cast<std::uint32_t>(type) != 0x10 && type != D3D12_COMMAND_LIST_TYPE_COPY) throw std::invalid_argument("DMA command list requires a copy allocator");
    auto nativeType = static_cast<std::uint32_t>(type) == 0x10 ? D3D12_COMMAND_LIST_TYPE_COPY : type;
    void* host = nullptr;
    auto* nativePipeline = initial && !initial->IsOffline() ? initial->GetHost() : nullptr;
    auto result = self->GetHost()->CreateCommandList(nodeMask, nativeType, allocation->GetHost(), nativePipeline, NativeGuid(GraphicsListId), &host);
    CheckGraphicsResult(result, "ID3D12Device::CreateCommandList");
    if (!host) throw std::runtime_error("D3D12 command list creation returned no interface");
    std::unique_ptr<ID3D12GraphicsCommandList, ListDeleter> ownedHost(static_cast<ID3D12GraphicsCommandList*>(host));
    auto wrapper = std::make_unique<CommandList>(ownedHost.get(), self, allocation, type, flags, initial);
    ownedHost.release();
    *list = wrapper.release();
    return result;
}
}

HRESULT AnyBox::Graphics::Device::CreateCommandList(Device* self, std::uint32_t nodeMask, D3D12_COMMAND_LIST_TYPE type, void* allocator, void* initialState, const Guid& interfaceId, void** list) { return CreateList(self, nodeMask, type, allocator, initialState, interfaceId, list, 0); }

HRESULT AnyBox::Graphics::Device::CreateCommandListX(Device* self, const XboxListDescription* description, void* allocator, void* initialState, const Guid& interfaceId, void** list) {
    if (!list) throw std::invalid_argument("Xbox command list output is null");
    *list = nullptr;
    if (!description) throw std::invalid_argument("Xbox command list description is null");
    if ((description->flags & ~std::uint32_t{6}) != 0 || description->barrierBatchSize != 0) throw std::runtime_error("Xbox command list recording flags and barrier batch configuration are not implemented");
    return CreateList(self, description->nodeMask, description->type, allocator, initialState, interfaceId, list, description->flags);
}
