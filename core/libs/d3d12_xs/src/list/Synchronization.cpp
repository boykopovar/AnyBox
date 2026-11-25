#include "List.hpp"
#include <stdexcept>
#include <utility>

namespace {
void ValidateRange(std::uint64_t address, std::uint64_t size) {
    constexpr auto addressLimit = std::uint64_t{1} << 48;
    if (size == 0 || address >= addressLimit || size > addressLimit - address || ((address | size) & 255) != 0) throw std::invalid_argument("GPU synchronization address range is invalid");
    static const auto applicationRange = [] {
        SYSTEM_INFO information{};
        GetSystemInfo(&information);
        return std::pair{reinterpret_cast<std::uintptr_t>(information.lpMinimumApplicationAddress), reinterpret_cast<std::uintptr_t>(information.lpMaximumApplicationAddress)};
    }();
    if (address > applicationRange.first || address + size <= applicationRange.second) throw std::runtime_error("Partial-address GPU synchronization is not implemented");
}

void ValidateFlags(D3D12_COMMAND_LIST_TYPE type, bool top, std::uint32_t flags, std::uint32_t control) {
    if (type != D3D12_COMMAND_LIST_TYPE_DIRECT && type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("GPU pipeline synchronization is not implemented for this command list type");
    if (flags == 0 || (control & 0x30000) == 0x30000) throw std::invalid_argument("GPU synchronization flags are inconsistent");
    auto allowed = top ? 0x8000DFBFU : 0x8010DF30U;
    auto allowedControl = 0x30000U;
    if (type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        if (top) { allowed |= 0x40; allowedControl |= 0x80000000; }
        else allowed |= 0x600000;
    }
    if ((flags & ~allowed) != 0 || (control & ~allowedControl) != 0) throw std::runtime_error("GPU synchronization flag mode is not implemented");
}
}

ID3D12GraphicsCommandList7* AnyBox::Graphics::CommandList::getSynchronizationHost() {
    if (synchronizationHost) return synchronizationHost;
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
    CheckGraphicsResult(device->GetHost()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options)), "D3D12 enhanced barrier feature query");
    if (!options.EnhancedBarriersSupported) throw std::runtime_error("D3D12 enhanced barriers are required for Xbox pipeline synchronization");
    constexpr GUID interfaceId{0xDD171223, 0x8B61, 0x4769, {0x90, 0xE3, 0x16, 0x0C, 0xCD, 0xE4, 0xE2, 0xC1}};
    void* object = nullptr;
    CheckGraphicsResult(host->QueryInterface(interfaceId, &object), "ID3D12GraphicsCommandList7 query");
    if (!object) throw std::runtime_error("D3D12 synchronization interface is null");
    synchronizationHost = static_cast<ID3D12GraphicsCommandList7*>(object);
    return synchronizationHost;
}

void AnyBox::Graphics::CommandList::synchronize(bool top, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size) {
    if (IsClosed()) throw std::logic_error("GPU synchronization requires a recording command list");
    flushGuestCommands();
    if (!GetPreparedDispatches().empty()) throw std::runtime_error("Mixed native and prepared compute commands are not implemented");
    ValidateFlags(guest.type, top, flags, control);
    ValidateRange(address, size);
    D3D12_GLOBAL_BARRIER barrier{D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_ACCESS_COMMON, D3D12_BARRIER_ACCESS_COMMON};
    D3D12_BARRIER_GROUP group{};
    group.Type = D3D12_BARRIER_TYPE_GLOBAL;
    group.NumBarriers = 1;
    group.pGlobalBarriers = &barrier;
    getSynchronizationHost()->Barrier(1, &group);
    nativeCommandsRecorded = true;
}

void AnyBox::Graphics::CommandList::SyncTopOfPipeX(CommandList* self, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size) { self->synchronize(true, flags, control, address, size); }
void AnyBox::Graphics::CommandList::SyncBottomOfPipeX(CommandList* self, std::uint32_t flags, std::uint32_t control, std::uint64_t address, std::uint64_t size) { self->synchronize(false, flags, control, address, size); }
