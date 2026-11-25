#include <d3d12_xs.hpp>
#include "../host/Library.hpp"
#include "Container.hpp"
#include <d3d12.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
struct BlobDeleter {
    void operator()(ID3DBlob* value) const noexcept { value->Release(); }
};
using Blob = std::unique_ptr<ID3DBlob, BlobDeleter>;
}

long D3D12SerializeVersionedRootSignature(const D3D12VersionedRootSignatureDescription* description, ID3DBlob** blob, ID3DBlob** errorBlob) noexcept(false) {
    if (!blob) throw std::invalid_argument("Root signature blob output is null");
    *blob = nullptr;
    if (errorBlob) *errorBlob = nullptr;
    if (!description) throw std::invalid_argument("Root signature description is null");
    auto flags = AnyBox::Graphics::GetRootSignatureFlags(*description);
    auto hostDescription = *description;
    if ((flags & AnyBox::Graphics::PreservedRootSignatureFlags) != 0) {
        if ((flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE) != 0) throw std::invalid_argument("A local root signature cannot contain additional root signature flags");
        AnyBox::Graphics::SetRootSignatureFlags(hostDescription, flags & ~AnyBox::Graphics::PreservedRootSignatureFlags);
    }

    static auto serialize = AnyBox::Graphics::GetHostLibrary().Get<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>("D3D12SerializeVersionedRootSignature");
    ID3DBlob* output = nullptr;
    ID3DBlob* diagnostics = nullptr;
    auto result = serialize(&hostDescription, &output, &diagnostics);
    Blob ownedOutput(output);
    Blob ownedDiagnostics(diagnostics);
    if (FAILED(result)) {
        std::string message = "D3D12SerializeVersionedRootSignature failed";
        if (diagnostics && diagnostics->GetBufferSize() != 0) {
            auto* data = static_cast<const char*>(diagnostics->GetBufferPointer());
            auto size = diagnostics->GetBufferSize();
            if (data[size - 1] == '\0') --size;
            message += ": " + std::string(data, size);
        }
        throw std::system_error(static_cast<int>(result), std::system_category(), message);
    }
    if (!output) throw std::runtime_error("Root signature serializer returned no blob");
    if ((flags & AnyBox::Graphics::PreservedRootSignatureFlags) != 0) AnyBox::Graphics::SetSerializedRootFlags(*output, flags);
    *blob = ownedOutput.release();
    if (errorBlob) *errorBlob = ownedDiagnostics.release();
    return result;
}
