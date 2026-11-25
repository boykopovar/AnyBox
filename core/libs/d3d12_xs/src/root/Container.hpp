#pragma once

#include <cstdint>
#include <d3d12.h>

namespace AnyBox::Graphics {
std::uint32_t GetRootSignatureFlags(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& description);
void SetRootSignatureFlags(D3D12_VERSIONED_ROOT_SIGNATURE_DESC& description, std::uint32_t flags);
void SetSerializedRootFlags(ID3DBlob& blob, std::uint32_t flags);
inline constexpr std::uint32_t XboxRootSignatureFlags = 0x83000000;
inline constexpr std::uint32_t PreservedRootSignatureFlags = XboxRootSignatureFlags | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
}
