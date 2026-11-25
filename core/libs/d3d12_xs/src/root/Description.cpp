#include "Description.hpp"
#include "Container.hpp"
#include "Serialized.hpp"
#include "../host/Library.hpp"
#include <memory>

AnyBox::Graphics::RootDescription::RootDescription(std::span<const std::byte> data) : flags(ReadSerializedRootFlags(data)), layout(Container::ReadRootSignatureLayout(data)), sourceDigest(Shader::HashSerializedRootSignature(data)) {
    if ((flags & ~0x83000FFFU) != 0) throw std::invalid_argument("Unsupported Xbox root signature flags");
    if ((flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE) != 0 && flags != D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE) throw std::invalid_argument("A local root signature cannot contain additional root signature flags");
    static auto deserialize = GetHostLibrary().Get<PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateVersionedRootSignatureDeserializer");
    constexpr GUID interfaceId{0x7F91CE67, 0x090C, 0x4BB7, {0xB7, 0x8E, 0xED, 0x8F, 0xF2, 0xE3, 0x1D, 0xA0}};
    void* output = nullptr;
    auto result = deserialize(data.data(), data.size(), interfaceId, &output);
    auto release = [](ID3D12VersionedRootSignatureDeserializer* value) { value->Release(); };
    std::unique_ptr<ID3D12VersionedRootSignatureDeserializer, decltype(release)> owned(static_cast<ID3D12VersionedRootSignatureDeserializer*>(output), release);
    Windows::CheckHResult(result, "D3D12CreateVersionedRootSignatureDeserializer");
    if (!output) throw std::runtime_error("Versioned root signature deserializer returned no interface");
    auto* value = owned->GetUnconvertedRootSignatureDesc();
    if (!value) throw std::runtime_error("Versioned root signature deserializer returned no description");
    description = *value;
    SetRootSignatureFlags(description, flags);
    owner = owned.release();
}
