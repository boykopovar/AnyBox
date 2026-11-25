#include <d3d12_xs.hpp>
#include "../host/Library.hpp"
#include "Container.hpp"
#include "Deserializer.hpp"
#include "Serialized.hpp"
#include <d3d12.h>
#include <memory>
#include <stdexcept>

namespace {
struct Release {
    template<class TInterface>
    void operator()(TInterface* value) const noexcept { value->Release(); }
};
}

long D3D12CreateRootSignatureDeserializer(const void* sourceData, std::size_t sourceDataSize, const _GUID& interfaceId, void** deserializer) noexcept(false) {
    if (!deserializer) throw std::invalid_argument("Root signature deserializer output is null");
    *deserializer = nullptr;
    if (!sourceData || sourceDataSize == 0) throw std::invalid_argument("Root signature source data is empty");
    static auto deserialize = AnyBox::Graphics::GetHostLibrary().Get<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateRootSignatureDeserializer");
    void* output = nullptr;
    auto result = deserialize(sourceData, sourceDataSize, interfaceId, &output);
    std::unique_ptr<IUnknown, Release> owned(static_cast<IUnknown*>(output));
    if (FAILED(result)) throw std::system_error(static_cast<int>(result), std::system_category(), "D3D12CreateRootSignatureDeserializer");
    if (!output) throw std::runtime_error("Root signature deserializer returned no interface");
    auto bytes = std::span(static_cast<const std::byte*>(sourceData), sourceDataSize);
    auto flags = AnyBox::Graphics::ReadSerializedRootFlags(bytes);
    if ((flags & ~0x83000FFFU) != 0) throw std::invalid_argument("Unsupported Xbox root signature flags");
    if ((flags & AnyBox::Graphics::PreservedRootSignatureFlags) == 0) {
        *deserializer = owned.release();
        return result;
    }

    ID3D12RootSignatureDeserializer* native = nullptr;
    auto query = owned->QueryInterface(AnyBox::Graphics::RootDeserializerId, reinterpret_cast<void**>(&native));
    std::unique_ptr<ID3D12RootSignatureDeserializer, Release> typed(native);
    if (FAILED(query)) throw std::system_error(static_cast<int>(query), std::system_category(), "Root signature deserializer query failed");
    if (!native) throw std::runtime_error("Root signature deserializer query returned no interface");
    auto* wrapper = new AnyBox::Graphics::RootDeserializer(native, flags);
    typed.release();
    *deserializer = static_cast<ID3D12RootSignatureDeserializer*>(wrapper);
    return result;
}
