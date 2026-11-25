#pragma once

#include "../device/Device.hpp"
#include "../object/Lifetime.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid QueueId{0x0EC870A6, 0x5D7E, 0x4C22, {0x8C, 0xFC, 0x5B, 0xAA, 0xE0, 0x76, 0x16, 0xED}};
const ObjectTable& GetQueueTable();

class Queue : public ObjectLifetime<Queue> {
public:
    Queue(ID3D12CommandQueue* host, Device* device, const D3D12_COMMAND_QUEUE_DESC& description);
    ~Queue();
    static constexpr std::size_t GetVtableOffset() { return offsetof(Queue, vtable); }
    ID3D12CommandQueue* GetHost() const noexcept { return host; }
    void SetXboxSelection(std::uint32_t selection) noexcept { xboxSelection = selection; }
    static bool Supports(const Guid& interfaceId) noexcept;
    static D3D12_COMMAND_QUEUE_DESC GetDescription(Queue* self);
    static HRESULT GetTimestampFrequency(Queue* self, std::uint64_t* frequency);
    static HRESULT GetClockCalibration(Queue* self, std::uint64_t* gpuTimestamp, std::uint64_t* cpuTimestamp);
    static void SetMarker(Queue* self, std::uint32_t metadata, const void* data, std::uint32_t size);
    static void BeginEvent(Queue* self, std::uint32_t metadata, const void* data, std::uint32_t size);
    static void EndEvent(Queue* self);
    static HRESULT Signal(Queue* self, void* fence, std::uint64_t value);
    static HRESULT Wait(Queue* self, void* fence, std::uint64_t value);
    static void ExecuteCommandLists(Queue* self, std::uint32_t count, void* const* lists);

private:
    friend class ObjectLifetime<Queue>;
    const ObjectTable* vtable;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::atomic<std::uint32_t> references{1};
    ID3D12CommandQueue* host;
    Device* device;
    D3D12_COMMAND_QUEUE_DESC description;
    std::uint32_t xboxSelection = 0;
    std::mutex submissionMutex;
    std::uint32_t preparedQueue = Device::PreparedQueueCount;
    HRESULT submissionFailure = S_OK;
};

static_assert(std::is_standard_layout_v<Queue>);
static_assert(Queue::GetVtableOffset() == 0);
}
