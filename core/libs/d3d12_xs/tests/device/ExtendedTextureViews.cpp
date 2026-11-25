#include "Tests.hpp"
#include "../../src/descriptor/Descriptions.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void CheckExtendedTextureShaderViews(AnyBox::Graphics::Device* device) {
    using namespace AnyBox;
    using namespace Graphics;
    using namespace Graphics::Tests;
    auto create = Method<decltype(&Device::CreateShaderResourceViewX)>(device, 70);
    D3D12_RESOURCE_DESC resource{};
    resource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource.Width = 256;
    resource.Height = 128;
    resource.DepthOrArraySize = 1;
    resource.MipLevels = 1;
    resource.Format = DXGI_FORMAT_R8_UNORM;
    resource.SampleDesc = {1, 0};
    resource.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(0x105);
    resource.Flags = static_cast<D3D12_RESOURCE_FLAGS>(0x300000);
    XboxShaderViewDescription description{};
    description.view.Format = DXGI_FORMAT_R8_UNORM;
    description.view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    description.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    description.view.Texture2D.MipLevels = 1;
    description.address = 0x123456780000;
    alignas(16) std::array<std::uint32_t, 16> storage;
    auto destination = reinterpret_cast<std::uintptr_t>(storage.data() + 4);
    constexpr std::array<std::uint32_t, 8> expected{0x34567800, 0xC0100012, 0x801FC03F, 0xD0500FAC, 0, 0x80400000, 0, 0};
    for (auto control : {0u, 1u, 2u, 3u}) {
        storage.fill(UINT32_MAX);
        description.parameters[3] = control;
        create(device, &resource, &description, destination);
        Require(std::ranges::equal(std::span(storage).subspan(4, 8), expected), "Extended R8 texture shader view differs from its hardware descriptor");
        Require(std::ranges::all_of(std::span(storage).first(4), [](auto value) { return value == UINT32_MAX; }) && std::ranges::all_of(std::span(storage).last(4), [](auto value) { return value == UINT32_MAX; }), "Extended texture shader view overwrote adjacent descriptors");
    }
    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Extended texture descriptor heap creation failed");
    auto release = [](DescriptorHeap* value) { Method<decltype(&DescriptorHeap::Release)>(value, 2)(value); };
    std::unique_ptr<DescriptorHeap, decltype(release)> heap(static_cast<DescriptorHeap*>(output), release);
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap.get(), 9)(heap.get());
    create(device, &resource, &description, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(expected))), "GPU did not observe the extended texture descriptor");

    resource.Width = 512;
    resource.Height = 256;
    resource.DepthOrArraySize = 6;
    resource.MipLevels = 5;
    resource.Format = description.view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(0x11B);
    description.view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    description.view.Texture2DArray = {1, UINT32_MAX, 2, 3, 0, 0};
    create(device, &resource, &description, destination);
    Require(((storage[7] >> 12) & 15) == 1 && ((storage[7] >> 16) & 15) == 4 && storage[8] == 0x20004 && storage[9] == 0x80400040, "Extended texture array shader view changed its mip or layer interval");
    auto previous = storage;
    auto reject = [&](auto change) {
        auto invalidResource = resource;
        auto invalidDescription = description;
        change(invalidResource, invalidDescription);
        Reject<std::exception>([&] { create(device, &invalidResource, &invalidDescription, destination); });
        Require(storage == previous, "Rejected extended texture view changed its output");
    };
    reject([](auto&, auto& value) { value.parameters[0] = 1; });
    reject([](auto&, auto& value) { value.parameters[1] = 1; });
    reject([](auto&, auto& value) { value.parameters[2] = 1; });
    reject([](auto&, auto& value) { value.parameters[3] = 4; });
    reject([](auto&, auto& value) { value.view.Texture2DArray.ArraySize = 7; });
    reject([](auto&, auto& value) { value.view.Texture2DArray.MostDetailedMip = 5; });
    reject([](auto&, auto& value) { value.view.Texture2DArray.PlaneSlice = 1; });
    reject([](auto&, auto& value) { value.view.Texture2DArray.ResourceMinLODClamp = 2; });
    reject([](auto&, auto& value) { value.view.Format = DXGI_FORMAT_R32_FLOAT; });
    reject([](auto&, auto& value) { value.address += 1; });
    reject([](auto&, auto& value) { value.address = UINT64_MAX; });
    reject([](auto& value, auto&) { value.Flags = D3D12_RESOURCE_FLAG_NONE; });
    reject([](auto& value, auto&) { value.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; });
    reject([](auto& value, auto&) { value.SampleDesc.Count = 2; });
    reject([](auto& value, auto&) { value.Width = UINT64_MAX; });
    reject([](auto& value, auto&) { value.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(0x11F); });
}
