#include "Queue.hpp"
#include "../fence/Fence.hpp"
#include "../host/Com.hpp"
#include "../list/List.hpp"
#include "../pipeline/Offline.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <anybox/vulkan/Timeline.hpp>
#include <array>
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using AnyBox::Graphics::CheckGraphicsResult;
using AnyBox::Graphics::ComOwner;
using AnyBox::Graphics::Device;
using AnyBox::Graphics::FenceId;
using AnyBox::Graphics::NativeGuid;
using AnyBox::Graphics::PreparedDispatch;
using AnyBox::Graphics::Queue;
namespace Vulkan = AnyBox::Vulkan;

constexpr std::uint64_t hostReady = 1;
constexpr std::uint64_t gpuComplete = 2;
constexpr std::uint64_t gpuResourcesReleased = 3;
constexpr std::uint64_t hostComplete = 4;
using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;

struct ReleaseQueue {
    void operator()(Queue* queue) const noexcept { Queue::Release(queue); }
};

class PreparedSubmission {
public:
    PreparedSubmission(Queue* owner, Vulkan::Context& context, std::vector<std::shared_ptr<const PreparedDispatch>> dispatches) : context(context), dispatches(std::move(dispatches)) {
        Queue::AddRef(owner);
        queue.reset(owner);
        owner->GetHost()->AddRef();
        hostQueue.reset(owner->GetHost());
    }

    ~PreparedSubmission() {
        if (wait) CloseThreadpoolWait(wait);
        ReleaseGpuResources();
    }

    void PrepareSubmission(Device& device) {
        void* nativeFence = nullptr;
        auto* host = device.GetHost();
        CheckGraphicsResult(host->CreateFence(0, D3D12_FENCE_FLAG_SHARED, NativeGuid(FenceId), &nativeFence), "Create prepared compute queue fence");
        bridge.reset(static_cast<ID3D12Fence*>(nativeFence));
        if (!bridge) throw std::runtime_error("Prepared compute queue fence creation returned no interface");
        HANDLE shared = nullptr;
        CheckGraphicsResult(host->CreateSharedHandle(bridge.get(), nullptr, GENERIC_ALL, nullptr, &shared), "Export prepared compute queue fence");
        Handle sharedHandle(shared, CloseHandle);
        timeline.emplace(Vulkan::Timeline::ImportD3D12Fence(context, sharedHandle.get()));

        event.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        if (!event) AnyBox::Windows::ThrowLastError("Create prepared compute queue completion event");
        HMODULE library = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&hostReady), &library)) {
            AnyBox::Windows::ThrowLastError("Retain the prepared compute queue library");
        }
        module.reset(library);
        TP_CALLBACK_ENVIRON environment;
        InitializeThreadpoolEnvironment(&environment);
        SetThreadpoolCallbackLibrary(&environment, module.get());
        wait = CreateThreadpoolWait(OnCompletion, this, &environment);
        DestroyThreadpoolEnvironment(&environment);
        if (!wait) AnyBox::Windows::ThrowLastError("Create prepared compute queue completion wait");

        const auto& functions = context.GetFunctions();
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = context.GetQueueFamily();
        Vulkan::Check(functions.createCommandPool(context.GetDevice(), &poolInfo, nullptr, &pool), "Create prepared compute command pool");
        VkCommandBufferAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocation.commandPool = pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        Vulkan::Check(functions.allocateCommandBuffers(context.GetDevice(), &allocation, &command), "Allocate prepared compute command buffer");
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Vulkan::Check(functions.beginCommandBuffer(command, &begin), "Begin prepared compute command buffer");
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        functions.pipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        invocations.reserve(dispatches.size());
        for (const auto& dispatch : dispatches) {
            if (!invocations.empty()) {
                functions.pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
            }
            invocations.push_back(dispatch->Record(command));
        }
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        functions.pipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        Vulkan::Check(functions.endCommandBuffer(command), "End prepared compute command buffer");
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        Vulkan::Check(functions.createFence(context.GetDevice(), &fenceInfo, nullptr, &fence), "Create prepared compute Vulkan fence");
    }

    ID3D12Fence* GetHostFence() const noexcept { return bridge.get(); }

    void Submit(std::uint32_t index) {
        auto semaphore = timeline->GetHandle();
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkTimelineSemaphoreSubmitInfo values{};
        values.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        values.waitSemaphoreValueCount = 1;
        values.pWaitSemaphoreValues = &hostReady;
        values.signalSemaphoreValueCount = 1;
        values.pSignalSemaphoreValues = &gpuComplete;
        VkSubmitInfo submission{};
        submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submission.pNext = &values;
        submission.waitSemaphoreCount = 1;
        submission.pWaitSemaphores = &semaphore;
        submission.pWaitDstStageMask = &stage;
        submission.commandBufferCount = 1;
        submission.pCommandBuffers = &command;
        submission.signalSemaphoreCount = 1;
        submission.pSignalSemaphores = &semaphore;
        context.Submit(index, submission, fence);
        gpuPending = true;
    }

    void RetireSubmission(bool nativeWaitQueued, bool markerMissing) noexcept {
        hostWaiting = nativeWaitQueued;
        retryMarker = markerMissing;
        ArmCompletion(gpuPending ? gpuComplete : hostReady);
    }

private:
    void ReleaseGpuResources() noexcept {
        if (fence) context.GetFunctions().destroyFence(context.GetDevice(), std::exchange(fence, VK_NULL_HANDLE), nullptr);
        if (pool) context.GetFunctions().destroyCommandPool(context.GetDevice(), std::exchange(pool, VK_NULL_HANDLE), nullptr);
        timeline.reset();
        invocations.clear();
        dispatches.clear();
        queue.reset();
    }

    void ArmCompletion(std::uint64_t value) noexcept {
        if ((value == hostComplete && retryMarker) || FAILED(bridge->SetEventOnCompletion(value, event.get()))) {
            ArmRetry();
        } else {
            SetThreadpoolWait(wait, event.get(), nullptr);
        }
    }

    void ArmRetry() noexcept {
        constexpr std::uint64_t relative = std::uint64_t{0} - 1'000'000;
        FILETIME delay{static_cast<DWORD>(relative), static_cast<DWORD>(relative >> 32)};
        SetThreadpoolWait(wait, event.get(), &delay);
    }

    static void CALLBACK OnCompletion(PTP_CALLBACK_INSTANCE instance, void* pointer, PTP_WAIT, TP_WAIT_RESULT) noexcept {
        auto* submission = static_cast<PreparedSubmission*>(pointer);
        auto completed = submission->bridge->GetCompletedValue();
        if (submission->gpuPending) {
            if (completed < gpuComplete) {
                submission->ArmRetry();
                return;
            }
            const auto& gpu = submission->context;
            auto result = gpu.GetFunctions().waitForFences(gpu.GetDevice(), 1, &submission->fence, VK_TRUE, 1'000'000);
            if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) {
                submission->ArmRetry();
                return;
            }
            submission->ReleaseGpuResources();
            submission->gpuPending = false;
        }
        if (submission->hostWaiting && !submission->hostResumed && completed < hostComplete) {
            if (FAILED(submission->bridge->Signal(gpuResourcesReleased))) {
                submission->ArmRetry();
                return;
            }
            submission->hostResumed = true;
            submission->ArmCompletion(hostComplete);
            return;
        }
        auto target = submission->hostWaiting ? hostComplete : hostReady;
        if (completed < target) {
            if (submission->retryMarker && SUCCEEDED(submission->hostQueue->Signal(submission->bridge.get(), hostComplete))) {
                submission->retryMarker = false;
                submission->ArmCompletion(hostComplete);
                return;
            }
            submission->ArmRetry();
            return;
        }
        FreeLibraryWhenCallbackReturns(instance, submission->module.release());
        delete submission;
    }

    std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> module{nullptr, FreeLibrary};
    std::unique_ptr<Queue, ReleaseQueue> queue;
    ComOwner<ID3D12CommandQueue> hostQueue;
    Vulkan::Context& context;
    std::vector<std::shared_ptr<const PreparedDispatch>> dispatches;
    std::vector<std::shared_ptr<const void>> invocations;
    ComOwner<ID3D12Fence> bridge;
    std::optional<Vulkan::Timeline> timeline;
    Handle event{nullptr, CloseHandle};
    PTP_WAIT wait = nullptr;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool gpuPending = false;
    bool hostWaiting = false;
    bool hostResumed = false;
    bool retryMarker = false;
};
}

void AnyBox::Graphics::Queue::ExecuteCommandLists(Queue* self, std::uint32_t count, void* const* lists) {
    if (count != 0 && !lists) throw std::invalid_argument("Command list array is null");
    std::array<ID3D12CommandList*, 16> local;
    std::array<ID3D12CommandList*, 16> localSorted;
    std::span<ID3D12CommandList*> native(local);
    std::span<ID3D12CommandList*> sorted(localSorted);
    if (count > local.size()) {
        thread_local std::vector<ID3D12CommandList*> expanded;
        expanded.resize(static_cast<std::size_t>(count) * 2);
        native = std::span(expanded).first(count);
        sorted = std::span(expanded).subspan(count);
    }
    auto type = self->description.Type;
    if (static_cast<std::uint32_t>(type) == 0x10) type = D3D12_COMMAND_LIST_TYPE_COPY;
    bool hasNative = false;
    std::vector<std::shared_ptr<const PreparedDispatch>> prepared;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto* list = CommandList::From(lists[index]);
        if (list->GetParent() != self->device) throw std::invalid_argument("Command list and queue devices differ");
        if (!list->IsClosed()) throw std::logic_error("Recording command list cannot be executed");
        if (list->GetNativeType() != type) throw std::invalid_argument("Command list and queue types differ");
        native[index] = list->GetHost();
        auto dispatches = list->GetPreparedDispatches();
        if (dispatches.empty()) {
            hasNative = true;
        } else {
            prepared.insert(prepared.end(), dispatches.begin(), dispatches.end());
        }
    }
    if (count > 1) {
        auto values = sorted.first(count);
        std::ranges::copy(native.first(count), values.begin());
        std::ranges::sort(values, std::less<ID3D12CommandList*>{});
        if (std::ranges::adjacent_find(values) != values.end()) throw std::runtime_error("Repeated command lists within one batch are not implemented");
    }
    if (prepared.empty()) {
        std::scoped_lock lock(self->submissionMutex);
        CheckGraphicsResult(self->submissionFailure, "Prepared compute queue ordering");
        self->host->ExecuteCommandLists(count, count == 0 ? nullptr : native.data());
        return;
    }
    if (hasNative) throw std::runtime_error("Mixed native and prepared command lists within one batch are not implemented");
    if (type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Prepared dispatch requires a compute queue");
    auto work = std::make_unique<PreparedSubmission>(self, self->device->GetGpuContext(), std::move(prepared));
    work->PrepareSubmission(*self->device);

    std::scoped_lock lock(self->submissionMutex);
    CheckGraphicsResult(self->submissionFailure, "Prepared compute queue ordering");
    if (self->preparedQueue == Device::PreparedQueueCount) self->preparedQueue = self->device->ReservePreparedQueue();
    auto* bridge = work->GetHostFence();
    CheckGraphicsResult(self->host->Signal(bridge, hostReady), "Signal host work before prepared compute");
    try {
        work->Submit(self->preparedQueue);
    } catch (...) {
        work.release()->RetireSubmission(false, false);
        throw;
    }
    auto waitResult = self->host->Wait(bridge, gpuResourcesReleased);
    if (FAILED(waitResult)) {
        self->submissionFailure = waitResult;
        work.release()->RetireSubmission(false, false);
        CheckGraphicsResult(waitResult, "Wait for prepared compute on the host queue");
    } else {
        auto marker = self->host->Signal(bridge, hostComplete);
        work.release()->RetireSubmission(true, FAILED(marker));
        CheckGraphicsResult(marker, "Signal host queue completion after prepared compute");
    }
}
