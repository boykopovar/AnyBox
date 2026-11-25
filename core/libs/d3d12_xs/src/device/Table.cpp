#include "Device.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) {
    throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!Device method " + std::to_string(TSlot) + " is not implemented");
}

template<std::size_t... TSlots>
AnyBox::Graphics::DeviceTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    DeviceTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&Device::QueryInterface);
    result[1] = MethodAddress(&Device::AddRef);
    result[2] = MethodAddress(&Device::Release);
    result[3] = MethodAddress(&Device::GetPrivateData);
    result[4] = MethodAddress(&Device::SetPrivateData);
    result[5] = MethodAddress(&Device::SetPrivateDataInterface);
    result[6] = MethodAddress(&Device::SetName);
    result[7] = MethodAddress(&Device::GetNodeCount);
    result[8] = MethodAddress(&Device::CreateCommandQueue);
    result[9] = MethodAddress(&Device::CreateCommandAllocator);
    result[12] = MethodAddress(&Device::CreateCommandList);
    result[14] = MethodAddress(&Device::CreateDescriptorHeap);
    result[15] = MethodAddress(&Device::GetDescriptorHandleIncrementSize);
    result[16] = MethodAddress(&Device::CreateRootSignature);
    result[17] = MethodAddress(&Device::CreateConstantBufferView);
    result[18] = MethodAddress(&Device::CreateShaderResourceView);
    result[19] = MethodAddress(&Device::CreateUnorderedAccessView);
    result[20] = MethodAddress(&Device::CreateRenderTargetView);
    result[23] = MethodAddress(&Device::CopyDescriptors);
    result[24] = MethodAddress(&Device::CopyDescriptorsSimple);
    result[25] = MethodAddress(&Device::GetResourceAllocationInfo);
    result[26] = MethodAddress(&Device::GetCustomHeapProperties);
    result[27] = MethodAddress(&Device::CreateCommittedResource);
    result[36] = MethodAddress(&Device::CreateFence);
    result[37] = MethodAddress(&Device::GetDeviceRemovedReason);
    result[39] = MethodAddress(&Device::CreateQueryHeap);
    result[41] = MethodAddress(&Device::CreateCommandSignature);
    result[43] = MethodAddress(&Device::GetAdapterLuid);
    result[48] = MethodAddress(&Device::CreateCommandQueueX);
    result[61] = MethodAddress(&Device::CreateSamplerX);
    result[62] = MethodAddress(&Device::CreateComputePipelineStateX);
    result[67] = MethodAddress(&Device::DeserializeComputePipelineStateX);
    result[70] = MethodAddress(&Device::CreateShaderResourceViewX);
    result[73] = MethodAddress(&Device::CreateCommandListX);
    result[74] = MethodAddress(&Device::RegisterPagePoolX);
    result[75] = MethodAddress(&Device::UnregisterPagePoolX);
    result[78] = MethodAddress(&Device::SetFrameIntervalX);
    result[79] = MethodAddress(&Device::ScheduleFrameEventX);
    result[80] = MethodAddress(&Device::WaitFrameEventX);
    result[86] = MethodAddress(&Device::CreatePipelineStateX);
    result[127] = MethodAddress(&Device::CreateCommittedResource3);
    result[133] = MethodAddress(&Device::CreatePlacedResourceLayoutX);
    return result;
}
}

const AnyBox::Graphics::DeviceTable& AnyBox::Graphics::GetDeviceTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<DeviceTable>>{});
    return table;
}
