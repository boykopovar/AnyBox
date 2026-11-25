#include "List.hpp"
#include "Compute.hpp"
#include <stdexcept>

AnyBox::Graphics::CommandList::CommandList(ID3D12GraphicsCommandList* host, Device* device, Allocator* allocator, D3D12_COMMAND_LIST_TYPE type, std::uint32_t flags) : guest(&GetListTable(), type), host(host), device(device), allocator(allocator), state(flags) {
    Device::AddRef(device);
    Allocator::AddRef(allocator);
    resetGuestCommands();
}

AnyBox::Graphics::CommandList::~CommandList() {
    delete recording;
    delete compute;
    if (synchronizationHost) synchronizationHost->Release();
    host->Release();
    Allocator::Release(allocator);
    Device::Release(device);
}

AnyBox::Graphics::CommandList* AnyBox::Graphics::CommandList::From(void* object) {
    if (!object) throw std::invalid_argument("Command list is null");
    const ListTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetListTable()) throw std::invalid_argument("Foreign command list interface is unsupported");
    return static_cast<CommandList*>(object);
}

bool AnyBox::Graphics::CommandList::SupportsCreation(D3D12_COMMAND_LIST_TYPE type, const Guid& interfaceId) noexcept {
    if (static_cast<std::uint32_t>(type) == 0x10) return interfaceId == DmaListId;
    if (type == D3D12_COMMAND_LIST_TYPE_COPY) return interfaceId == GraphicsListId;
    if (type != D3D12_COMMAND_LIST_TYPE_DIRECT && type != D3D12_COMMAND_LIST_TYPE_BUNDLE && type != D3D12_COMMAND_LIST_TYPE_COMPUTE) return false;
    return interfaceId == GraphicsListId || interfaceId == XboxGraphicsListId;
}

bool AnyBox::Graphics::CommandList::Supports(const Guid& interfaceId) const noexcept { return interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == CommandListId || SupportsCreation(guest.type, interfaceId); }

D3D12_COMMAND_LIST_TYPE AnyBox::Graphics::CommandList::GetType(CommandList* self) { return self->guest.type; }
