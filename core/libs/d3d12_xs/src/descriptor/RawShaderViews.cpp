#include "Descriptions.hpp"
#include "RawBuffer.hpp"
#include "TypedBuffer.hpp"
#include "ExtendedTextureView.hpp"
#include "../device/Device.hpp"
#include <anybox/Unimplemented.hpp>
#include <algorithm>

void AnyBox::Graphics::Device::CreateShaderResourceViewX(Device*, const D3D12_RESOURCE_DESC* resource, const XboxShaderViewDescription* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox extended shader view destination is null or misaligned");
    auto* output = reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination));
    if (!resource) {
        std::memset(output, 0, 32);
        return;
    }
    if (!description) ANYBOX_NOT_IMPLEMENTED();
    if (resource->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
        auto words = EncodeTextureShaderViewX(*resource, *description);
        std::memcpy(output, words.data(), sizeof(words));
        return;
    }
    const auto& view = description->view;
    if (view.ViewDimension != D3D12_SRV_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
    if ((resource->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0) throw std::invalid_argument("Xbox resource does not allow shader resource views");
    if (view.Buffer.StructureByteStride != 0) throw std::invalid_argument("Xbox raw shader view cannot have a structure stride");
    if (description->address == UINT64_MAX || std::ranges::any_of(description->parameters, [](auto value) { return value != 0; })) ANYBOX_NOT_IMPLEMENTED();
    std::array<std::uint32_t, 8> words;
    if (view.Buffer.Flags == D3D12_BUFFER_SRV_FLAG_NONE) words = EncodeTypedBuffer(description->address, resource->Width, view);
    else if (view.Format == DXGI_FORMAT_R32_TYPELESS && view.Buffer.Flags == D3D12_BUFFER_SRV_FLAG_RAW) words = EncodeRawBuffer(description->address, resource->Width, view.Buffer.FirstElement, view.Buffer.NumElements, view.Shader4ComponentMapping);
    else ANYBOX_NOT_IMPLEMENTED();
    std::memcpy(output, words.data(), sizeof(words));
}
