#include <d3d12_xs.hpp>
#include <anybox/graphics/container/Dxbc.hpp>
#include <d3d12.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace {
constexpr GUID DeserializerId{0x34AB647B, 0x3CC8, 0x46AC, {0x84, 0x1B, 0xC0, 0x96, 0x56, 0x45, 0xC0, 0x46}};
struct Release {
    template<class TInterface>
    void operator()(TInterface* value) const noexcept { value->Release(); }
};

void Check(D3D_ROOT_SIGNATURE_VERSION version, std::uint32_t flags) {
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.Constants = {2, 3, 4};
    D3D12_ROOT_PARAMETER1 parameter1{};
    parameter1.ParameterType = parameter.ParameterType;
    parameter1.Constants = parameter.Constants;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
    description.Version = version;
    auto nativeFlags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags);
    if (version == D3D_ROOT_SIGNATURE_VERSION_1_0) description.Desc_1_0 = {1, &parameter, 0, nullptr, nativeFlags};
    else description.Desc_1_1 = {1, &parameter1, 0, nullptr, nativeFlags};

    ID3DBlob* output = nullptr;
    ID3DBlob* errors = nullptr;
    auto result = D3D12SerializeVersionedRootSignature(&description, &output, &errors);
    std::unique_ptr<ID3DBlob, Release> blob(output), diagnostics(errors);
    if (result != 0 || !blob || errors) throw std::runtime_error("Xbox root signature serialization failed");
    auto inputFlags = version == D3D_ROOT_SIGNATURE_VERSION_1_0 ? description.Desc_1_0.Flags : description.Desc_1_1.Flags;
    if (inputFlags != nativeFlags) throw std::runtime_error("Xbox root signature serialization modified the input flags");
    auto bytes = std::span(static_cast<const std::byte*>(blob->GetBufferPointer()), blob->GetBufferSize());
    auto checksum = AnyBox::Graphics::Container::ComputeDxbcChecksum(bytes.subspan(20));
    if (!std::ranges::equal(checksum, bytes.subspan(4, 16))) throw std::runtime_error("Xbox root signature serialization produced an invalid checksum");

    void* object = nullptr;
    D3D12CreateRootSignatureDeserializer(bytes.data(), bytes.size(), DeserializerId, &object);
    std::unique_ptr<ID3D12RootSignatureDeserializer, Release> restored(static_cast<ID3D12RootSignatureDeserializer*>(object));
    blob.reset();
    constexpr GUID unknownId{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    for (const auto& interfaceId : {DeserializerId, unknownId}) {
        void* identity = nullptr;
        if (restored->QueryInterface(interfaceId, &identity) != S_OK || identity != object) throw std::runtime_error("Xbox root signature deserializer identity changed");
        if (static_cast<IUnknown*>(identity)->Release() != 1) throw std::runtime_error("Xbox root signature deserializer reference count changed");
    }
    GUID unsupported{};
    void* invalid = object;
    bool rejected = false;
    try { restored->QueryInterface(unsupported, &invalid); } catch (const std::system_error&) { rejected = true; }
    if (!rejected || invalid) throw std::runtime_error("Xbox root signature deserializer exposed an unsupported interface");

    auto* value = restored->GetRootSignatureDesc();
    if (value->Flags != nativeFlags || value->NumParameters != 1) throw std::runtime_error("Xbox root signature flags or parameters were lost during deserialization");
    const auto& constants = value->pParameters[0].Constants;
    if (constants.Num32BitValues != 4 || constants.ShaderRegister != 2 || constants.RegisterSpace != 3) throw std::runtime_error("Xbox root signature constants were changed during serialization");
}

template<class TException>
void Reject(std::uint32_t flags) {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
    description.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    description.Desc_1_1.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags);
    ID3DBlob* output = nullptr;
    ID3DBlob* errors = nullptr;
    bool rejected = false;
    try { D3D12SerializeVersionedRootSignature(&description, &output, &errors); } catch (const TException&) { rejected = true; }
    std::unique_ptr<ID3DBlob, Release> blob(output), diagnostics(errors);
    if (!rejected || output || errors) throw std::runtime_error("Unsupported Xbox root signature flags were accepted");
}
}

void CheckXboxRootFlags() {
    for (auto version : {D3D_ROOT_SIGNATURE_VERSION_1_0, D3D_ROOT_SIGNATURE_VERSION_1_1}) {
        for (auto flags : {0x400U, 0x800U, 0xC00U, 0x01000000U, 0x02000000U, 0x03000000U, 0x80000000U, 0x83000000U, 0x83000C00U}) {
            Check(version, flags);
            Check(version, flags | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        }
    }
    Reject<std::system_error>(0x04000000);
    Reject<std::system_error>(0x03001000);
    Reject<std::invalid_argument>(0x03000080);
    Reject<std::invalid_argument>(0x480);
    Reject<std::invalid_argument>(0x880);
    Reject<std::invalid_argument>(0xC80);
}
