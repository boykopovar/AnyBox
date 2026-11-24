#ifndef ANYBOX_D3D12_XS_ROOT_SIGNATURE_HPP
#define ANYBOX_D3D12_XS_ROOT_SIGNATURE_HPP

#include <cstddef>
#include <cstdint>

namespace AnyBox::D3D12Xs {

bool HasHostUnknownValues(const void* description);
bool HasHostUnknownSerializedValues(const void* sourceData, std::size_t sourceDataSize);
std::int32_t SerializeVersionedRootSignature(const void* description, void** blob, void** errorBlob);
std::int32_t CreateRootSignatureDeserializer(const void* sourceData, std::size_t sourceDataSize, const void* interfaceId, void** deserializer);

}

#endif
