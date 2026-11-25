#pragma once

#include "../indirect/Description.hpp"

#include <d3d12_xsTypes.hpp>
#include <anybox/Types.hpp>
#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include "../object/Properties.hpp"
#include "Interfaces.hpp"
#include "../frame/Configuration.hpp"
#include "../queue/Slots.hpp"
#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <type_traits>
#include <anybox/texture/AddressSpace.hpp>
#include <anybox/buffer/AddressSpace.hpp>
#include <anybox/vulkan/Context.hpp>
#include <bit>

namespace AnyBox::Vulkan { class Context; class Buffer; }

namespace AnyBox::Descriptor { struct SamplerDescription; }

namespace AnyBox::Graphics {
struct SerializedComputeDescription;
using DeviceMethod = std::array<std::byte, sizeof(void (*)())>;
using ObjectTable = std::array<DeviceMethod, 128>;
using DeviceTable = std::array<DeviceMethod, 256>;

template<class TFunction>
DeviceMethod MethodAddress(TFunction function) {
    DeviceMethod result{};
    static_assert(sizeof(function) == sizeof(result));
    std::memcpy(result.data(), &function, sizeof(function));
    return result;
}

const DeviceTable& GetDeviceTable();
struct XboxQueueDescription;
struct XboxListDescription;
struct XboxShaderViewDescription;
class DxgiDevice;
class FrameClock;
class PagePools;
struct FrameAncillary;
GUID NativeGuid(const Guid& value);
void CheckGraphicsResult(HRESULT result, const char* operation);

class Device : public ObjectProperties<Device> {
public:
    Device(ID3D12Device* host, const D3D12XboxCreateDeviceParameters& parameters);
    ~Device();
    static bool Supports(const Guid& interfaceId) noexcept;
    static std::int32_t QueryInterface(Device* self, const Guid& interfaceId, void** object);
    static std::uint32_t AddRef(Device* self);
    static std::uint32_t Release(Device* self);
    ID3D12Device* GetHost() const noexcept { return host; }
    std::array<std::uint32_t, 2> GetWaveLimits();
    Vulkan::Context& GetGpuContext() const noexcept { return *gpuContext; }
    std::uint64_t GetNullConstantAddress() const noexcept { return nullConstantAddress; }
    Texture::AddressSpace& GetTextureAddresses() noexcept { return textureAddresses; }
    Buffers::AddressSpace& GetBufferAddresses() noexcept { return bufferAddresses; }
    Buffers::AddressSpace& GetDescriptorAddresses() noexcept { return descriptorAddresses; }
    std::uint32_t AcquireGpuQueue() { return gpuQueueSlots.Acquire(gpuContext->GetQueueCount()); }
    void ReleaseGpuQueue(std::uint32_t index) noexcept { gpuQueueSlots.Release(gpuContext->GetQueueCount(), index); }
    PagePools& GetPagePools() noexcept { return *pagePools; }
    static HRESULT RegisterPagePoolX(Device* self, std::uint64_t address, std::uint32_t count, std::uint64_t* handle);
    static HRESULT UnregisterPagePoolX(Device* self, std::uint64_t handle);
    bool Matches(const D3D12XboxCreateDeviceParameters& value) const noexcept;
    static std::uint32_t TryAddRef(Device* self);
    static std::uint32_t GetNodeCount(Device* self);
    static std::uint32_t GetDescriptorHandleIncrementSize(Device* self, D3D12_DESCRIPTOR_HEAP_TYPE type);
    static D3D12_HEAP_PROPERTIES GetCustomHeapProperties(Device* self, std::uint32_t nodeMask, D3D12_HEAP_TYPE type);
    static D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(Device* self, std::uint32_t nodeMask, std::uint32_t count, const D3D12_RESOURCE_DESC* descriptions);
    static HRESULT CreateCommittedResource3(Device* self, const D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1* description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue, void* protectedSession, std::uint32_t formatCount, const DXGI_FORMAT* formats, const Guid& interfaceId, void** resource);
    static HRESULT CreateCommandQueue(Device* self, const D3D12_COMMAND_QUEUE_DESC* description, const Guid& interfaceId, void** queue);
    static HRESULT CreateFence(Device* self, std::uint64_t initialValue, D3D12_FENCE_FLAGS flags, const Guid& interfaceId, void** fence);
    static HRESULT CreateCommandSignature(Device* self, const CommandSignatureDescription* description, void* rootSignature, const Guid& interfaceId, void** signature);
    static HRESULT CreateQueryHeap(Device* self, const D3D12_QUERY_HEAP_DESC* description, const Guid& interfaceId, void** heap);
    static HRESULT CreateCommittedResource(Device* self, const D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC* description, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue, const Guid& interfaceId, void** resource);
    static HRESULT CreateCommandQueueX(Device* self, const XboxQueueDescription* description, const Guid& interfaceId, void** queue);
    void ReleaseQueueSelection(std::uint32_t selection) noexcept;
    static HRESULT CreateCommandAllocator(Device* self, D3D12_COMMAND_LIST_TYPE type, const Guid& interfaceId, void** allocator);
    static HRESULT CreateCommandList(Device* self, std::uint32_t nodeMask, D3D12_COMMAND_LIST_TYPE type, void* allocator, void* initialState, const Guid& interfaceId, void** list);
    static HRESULT CreateCommandListX(Device* self, const XboxListDescription* description, void* allocator, void* initialState, const Guid& interfaceId, void** list);
    static HRESULT CreateRootSignature(Device* self, std::uint32_t nodeMask, const void* data, std::size_t size, const Guid& interfaceId, void** signature);
    static HRESULT CreatePipelineStateX(Device* self, const D3D12_PIPELINE_STATE_STREAM_DESC* description, const Guid& interfaceId, void** pipeline);
    static HRESULT CreateComputePipelineStateX(Device* self, const D3D12_COMPUTE_PIPELINE_STATE_DESC* description, std::uint32_t extensionCount, const void* extensions, const Guid& interfaceId, void** pipeline);
    static HRESULT DeserializeComputePipelineStateX(Device* self, const SerializedComputeDescription* description, const Guid& interfaceId, void** pipeline);
    static HRESULT CreateDescriptorHeap(Device* self, const D3D12_DESCRIPTOR_HEAP_DESC* description, const Guid& interfaceId, void** heap);
    static void CreateShaderResourceView(Device* self, void* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t destination);
    static void CreateShaderResourceViewX(Device* self, const D3D12_RESOURCE_DESC* resource, const XboxShaderViewDescription* description, std::uint64_t destination);
    static void CopyDescriptorsSimple(Device* self, std::uint32_t count, std::uint64_t destination, std::uint64_t source, D3D12_DESCRIPTOR_HEAP_TYPE type);
    static void CopyDescriptors(Device* self, std::uint32_t destinationCount, const std::uint64_t* destinations, const std::uint32_t* destinationSizes, std::uint32_t sourceCount, const std::uint64_t* sources, const std::uint32_t* sourceSizes, D3D12_DESCRIPTOR_HEAP_TYPE type);
    static void CreateSamplerX(Device* self, const Descriptor::SamplerDescription* description, std::uint64_t destination);
    static void CreateConstantBufferView(Device* self, const D3D12_CONSTANT_BUFFER_VIEW_DESC* description, std::uint64_t destination);
    static void CreateUnorderedAccessView(Device* self, void* resource, void* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* description, std::uint64_t destination);
    static void CreateRenderTargetView(Device* self, void* resource, const D3D12_RENDER_TARGET_VIEW_DESC* description, std::uint64_t destination);
    static HRESULT GetDeviceRemovedReason(Device* self);
    static LUID GetAdapterLuid(Device* self);
    static HRESULT CreatePlacedResourceLayoutX(Device* self, std::uintptr_t address, const D3D12_RESOURCE_DESC1* description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue, const Guid& interfaceId, void** resource);
    static HRESULT SetFrameIntervalX(Device* self, void* output, std::uint32_t intervalMicroseconds, std::uint32_t periodicity, std::uint32_t flags);
    static HRESULT ScheduleFrameEventX(Device* self, std::uint32_t type, std::uint32_t offsetMicroseconds, const FrameAncillary* ancillary, std::uint32_t flags);
    static HRESULT WaitFrameEventX(Device* self, std::uint32_t type, std::uint32_t timeoutMilliseconds, const FrameAncillary* ancillary, std::uint32_t flags, std::uint64_t* token);
    FrameConfiguration GetFrameConfiguration() const {
        std::scoped_lock lock(frameMutex);
        return frameConfiguration;
    }

private:
    friend class DxgiDevice;
    const DeviceTable* vtable;
    std::atomic<std::uint32_t> references{1};
    std::atomic<std::uint32_t> computeQueues{0};
    GpuQueueSlots gpuQueueSlots;
    ID3D12Device* host;
    D3D12XboxCreateDeviceParameters parameters;
    std::mutex dxgiMutex;
    DxgiDevice* dxgiDevice = nullptr;
    mutable std::mutex frameMutex;
    FrameConfiguration frameConfiguration;
    FrameClock* frameClock = nullptr;
    PagePools* pagePools = nullptr;
    Vulkan::Context* gpuContext = nullptr;
    Vulkan::Buffer* nullConstantBuffer = nullptr;
    std::uint64_t nullConstantAddress = 0;
    Texture::AddressSpace textureAddresses;
    Buffers::AddressSpace bufferAddresses;
    Buffers::AddressSpace descriptorAddresses;
    std::once_flag waveInitialization;
    std::array<std::uint32_t, 2> waveLimits{};
};

static_assert(std::is_standard_layout_v<Device>);
static_assert(sizeof(DeviceMethod) == 8);
Device* AcquireDevice(ID3D12Device* host, const D3D12XboxCreateDeviceParameters& parameters);
void ForgetDevice(Device* device);
}
