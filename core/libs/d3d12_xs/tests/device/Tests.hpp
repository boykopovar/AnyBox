#pragma once

#include "../../src/device/Device.hpp"
#include <anybox/ExceptionTest.hpp>
#include <cstring>
#include <stdexcept>
#include <span>
#include <vector>

namespace AnyBox::Graphics { class Resource; class RootSignature; }

namespace AnyBox::Graphics::Tests {
void CheckPipelineStreams(Device* device, RootSignature* root, std::span<const std::byte> code);
void CheckComputeRoots(Device* device);
void CheckComputeInline(Device* device);
void CheckPreparedPipeline(Device* device);
void CheckSerializedPipeline(Device* device);
void CheckSerializedGraphicsState(Device* device);
void CheckPreparedDispatch(Device* device);
inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TFunction, class TObject>
TFunction Method(TObject* device, std::size_t slot) {
    const std::byte* table;
    std::memcpy(&table, static_cast<const void*>(device), sizeof(table));
    TFunction result;
    static_assert(sizeof(result) == sizeof(DeviceMethod));
    std::memcpy(&result, table + slot * sizeof(DeviceMethod), sizeof(result));
    return result;
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    int destructors = 0;
    bool caught = false;
    try { AnyBox::Tests::Invoke(callable, destructors); } catch (const TException&) { caught = true; }
    Require(caught && destructors == 1, "Graphics device exception did not unwind its caller");
}

void CheckProperties(Device* device);
void CheckAllocation(Device* device);
void CheckCommittedTextures(Device* device);
void CheckCommittedBuffers(Device* device);
void CheckCommittedBufferGpu(Device* device);
void CheckCommittedBufferInvalid(Device* device);
void CheckCommittedTextureLegacy(Device* device);
void CheckQueues(Device* device);
void CheckFences(Device* device);
void CheckQueryHeaps(Device* device);
void CheckCommandSignatures(Device* device);
void CheckAllocators(Device* device);
void CheckLists(Device* device);
void CheckRoots(Device* device);
void CheckPipelines(Device* device);
void CheckPipelineBindings(Device* device, void* pipeline);
void CheckInitialPipelines(Device* device, void* pipeline);
std::vector<std::byte> CompileComputeShader();
void CheckRootLayouts(Device* device);
void CheckSynchronization(Device* device);
void CheckInlineCommands(Device* device);
void CheckDxgi(Device* device);
void CheckFrameConfiguration(Device* device);
void CheckFrameTimeline();
void CheckFrameEvents(Device* device);
void CheckFrameAncillary(Device* device);
void CheckUnsupportedDeviceMethods(Device* device);
void CheckPhysicalHeap(Device* device);
void CheckPagePools(Device* device);
void CheckPlacedBuffer(Device* device);
void CheckShaderBufferArguments(Device* device, Resource* resource);
void CheckDescriptorHeaps(Device* device);
void CheckSamplers(Device* device);
void CheckDescriptorCopies(Device* device);
void CheckNullShaderViews(Device* device);
void CheckNullUnorderedViews(Device* device);
void CheckConstantViews(Device* device);
void CheckRawShaderViews(Device* device);
void CheckTypedShaderViews(Device* device);
void CheckVulkanTimeline(Device* device);
void CheckGpuQueue(Device* device);
void CheckGpuQueueSlots(Device* device);
void CheckCommandArena(Device* device);
void CheckCommandRetirement(Device* device);
}
