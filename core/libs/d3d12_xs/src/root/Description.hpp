#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <anybox/graphics/container/RootSignature.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>

namespace AnyBox::Graphics {
class RootDescription {
public:
    explicit RootDescription(std::span<const std::byte> data);
    ~RootDescription() { owner->Release(); }
    RootDescription(const RootDescription&) = delete;
    RootDescription& operator=(const RootDescription&) = delete;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& GetDescription() const noexcept { return description; }
    std::uint32_t GetFlags() const noexcept { return flags; }
    const Container::RootSignatureLayout& GetLayout() const noexcept { return layout; }
    const Shader::ShaderDigest& GetSourceDigest() const noexcept { return sourceDigest; }

private:
    ID3D12VersionedRootSignatureDeserializer* owner;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC description;
    std::uint32_t flags;
    Container::RootSignatureLayout layout;
    Shader::ShaderDigest sourceDigest;
};
}
