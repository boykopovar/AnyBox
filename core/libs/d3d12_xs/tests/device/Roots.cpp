#include "Tests.hpp"
#include "../../src/root/Signature.hpp"
#include <d3d12_xs.hpp>
#include <array>
#include <memory>

namespace {
struct BlobDeleter {
    void operator()(ID3DBlob* value) const noexcept { value->Release(); }
};
}

void AnyBox::Graphics::Tests::CheckRoots(Device* device) {
    auto create = Method<decltype(&Device::CreateRootSignature)>(device, 16);
    for (auto version : {D3D_ROOT_SIGNATURE_VERSION_1_0, D3D_ROOT_SIGNATURE_VERSION_1_1}) {
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
        description.Version = version;
        if (version == D3D_ROOT_SIGNATURE_VERSION_1_0) description.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        else description.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ID3DBlob* data = nullptr;
        Require(D3D12SerializeVersionedRootSignature(&description, &data, nullptr) == 0 && data, "Device root signature test serialization failed");
        std::unique_ptr<ID3DBlob, BlobDeleter> blob(data);
        void* output = nullptr;
        Require(create(device, 0, data->GetBufferPointer(), data->GetBufferSize(), RootSignatureId, &output) == 0 && output, "Root signature device creation failed");
        auto* signature = static_cast<RootSignature*>(output);
        auto serialized = std::span(static_cast<const std::byte*>(data->GetBufferPointer()), data->GetBufferSize());
        Require(signature->GetSourceDigest() == Shader::HashSerializedRootSignature(serialized), "Root signature source digest changed during host translation");
        auto query = Method<decltype(&RootSignature::QueryInterface)>(signature, 0);
        auto release = Method<decltype(&RootSignature::Release)>(signature, 2);
        for (const auto& id : {RootSignatureId, UnknownId, ObjectId, DeviceChildId}) {
            void* identity = nullptr;
            Require(query(signature, id, &identity) == 0 && identity == signature, "Root signature identity changed across the Xbox ABI");
            Require(release(static_cast<RootSignature*>(identity)) == 1, "Root signature query did not retain its reference");
        }
        Require(Method<decltype(&RootSignature::GetDevice)>(signature, 7)(signature, BaseDeviceId, &output) == 0 && output == device, "Root signature parent identity changed");
        Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(output));
        Reject<std::invalid_argument>([&] { query(signature, PageableId, &output); });
        Require(!output, "Root signature incorrectly exposed a pageable interface");
        Require(Method<decltype(&RootSignature::SetName)>(signature, 6)(signature, L"AnyBox root signature ABI test") == 0, "Root signature naming failed");
        Require(release(signature) == 0, "Root signature final reference was not released");
    }
    void* output = nullptr;
    Reject<std::invalid_argument>([&] { create(device, 0, nullptr, 0, RootSignatureId, &output); });
    const std::array<std::byte, 8> malformed{};
    Reject<std::invalid_argument>([&] { create(device, 0, malformed.data(), malformed.size(), RootSignatureId, &output); });
    Require(!output, "Malformed root signature retained an output interface");
    CheckRootLayouts(device);
}
