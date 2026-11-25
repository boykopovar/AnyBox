#pragma once

#include "Tests.hpp"
#include "../../src/queue/Queue.hpp"
#include "../../src/fence/Fence.hpp"

namespace AnyBox::Graphics::Tests {
class QueueCommands {
public:
    explicit QueueCommands(Vulkan::Context& context) : context(context) {
        VkCommandPoolCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        description.queueFamilyIndex = context.GetQueueFamily();
        Vulkan::Check(context.GetFunctions().createCommandPool(context.GetDevice(), &description, nullptr, &pool), "Create reusable queue test command pool");
    }
    ~QueueCommands() { context.GetFunctions().destroyCommandPool(context.GetDevice(), pool, nullptr); }
    VkCommandBuffer Begin() {
        VkCommandBufferAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocation.commandPool = pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        VkCommandBuffer commands = VK_NULL_HANDLE;
        Vulkan::Check(context.GetFunctions().allocateCommandBuffers(context.GetDevice(), &allocation, &commands), "Allocate reusable queue test commands");
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        Vulkan::Check(context.GetFunctions().beginCommandBuffer(commands, &begin), "Begin reusable queue test commands");
        return commands;
    }

private:
    Vulkan::Context& context;
    VkCommandPool pool = VK_NULL_HANDLE;
};

struct QueueRelease {
    void operator()(Queue* value) const noexcept { Method<decltype(&Queue::Release)>(value, 2)(value); }
};
using QueueOwner = std::unique_ptr<Queue, QueueRelease>;

inline QueueOwner CreateGpuQueue(Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) {
    D3D12_COMMAND_QUEUE_DESC description{type, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateCommandQueue)>(device, 8)(device, &description, QueueId, &output) == S_OK && output, "GPU queue bridge wrapper creation failed");
    return QueueOwner(static_cast<Queue*>(output));
}

struct FenceRelease {
    void operator()(Fence* value) const noexcept { Method<decltype(&Fence::Release)>(value, 2)(value); }
};
struct EventRelease {
    void operator()(void* value) const noexcept { CloseHandle(value); }
};

class GpuQueueFixture {
public:
    explicit GpuQueueFixture(Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) : event(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {
        Require(event != nullptr, "GPU queue bridge completion event creation failed");
        void* output = nullptr;
        Require(Method<decltype(&Device::CreateFence)>(device, 36)(device, 0, D3D12_FENCE_FLAG_SHARED, FenceId, &output) == S_OK && output, "GPU queue bridge completion fence creation failed");
        fence.reset(static_cast<Fence*>(output));
        queue = CreateGpuQueue(device, type);
    }
    Queue& GetQueue() const noexcept { return *queue; }
    Fence& GetFence() const noexcept { return *fence; }
    void Signal(std::uint64_t value) { Require(Method<decltype(&Queue::Signal)>(queue.get(), 14)(queue.get(), fence.get(), value) == S_OK, "GPU queue bridge client signal failed"); }
    void Wait(Fence& dependency, std::uint64_t value) { Require(Method<decltype(&Queue::Wait)>(queue.get(), 15)(queue.get(), &dependency, value) == S_OK, "GPU queue bridge client wait failed"); }
    void Register(std::uint64_t value) { Require(Method<decltype(&Fence::SetEventOnCompletion)>(fence.get(), 9)(fence.get(), value, event.get()) == S_OK, "GPU queue bridge completion registration failed"); }
    DWORD Await(DWORD milliseconds) const { return WaitForSingleObject(event.get(), milliseconds); }

private:
    std::unique_ptr<void, EventRelease> event;
    std::unique_ptr<Fence, FenceRelease> fence;
    QueueOwner queue;
};

struct QueueGate {
    Fence& fence;
    std::uint64_t value;
    ~QueueGate() { if (FAILED(fence.GetHost()->Signal(value))) std::terminate(); }
};
}
