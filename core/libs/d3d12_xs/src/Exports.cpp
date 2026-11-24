#include <d3d12_xs.hpp>
#include <anybox/Unimplemented.hpp>

#include "RootSignature.hpp"

std::int32_t D3D12ConfigureVirtualMemoryX(const D3D12VirtualMemoryConfiguration*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t D3D12CreateRootSignatureDeserializer(const void* sourceData, std::size_t sourceDataSize, const AnyBox::Guid& interfaceId, void** deserializer) noexcept(false) {
    if (AnyBox::D3D12Xs::HasHostUnknownSerializedValues(sourceData, sourceDataSize)) ANYBOX_NOT_IMPLEMENTED();
    return AnyBox::D3D12Xs::CreateRootSignatureDeserializer(sourceData, sourceDataSize, &interfaceId, deserializer);
}

std::int32_t D3D12SerializeVersionedRootSignature(const D3D12VersionedRootSignatureDescription* description, ID3DBlob** blob, ID3DBlob** errorBlob) noexcept(false) {
    if (AnyBox::D3D12Xs::HasHostUnknownValues(description)) ANYBOX_NOT_IMPLEMENTED();
    return AnyBox::D3D12Xs::SerializeVersionedRootSignature(description, reinterpret_cast<void**>(blob), reinterpret_cast<void**>(errorBlob));
}

std::int32_t D3D12XboxCreateDevice(IGraphicsUnknown*, const D3D12XboxCreateDeviceParameters*, const AnyBox::Guid&, void**) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}
