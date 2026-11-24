#include <anybox/ExceptionTest.hpp>

#include "RootSignature.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#else
#error Root signature tests require Windows
#endif

namespace {

using Microsoft::WRL::ComPtr;

constexpr UINT tableParameter = 0;
constexpr UINT constantsParameter = 1;
constexpr UINT descriptorParameter = 2;
constexpr UINT parameterCount = 3;
constexpr UINT rangeCount = 2;
constexpr auto rootSignatureVersion1_2 = static_cast<D3D_ROOT_SIGNATURE_VERSION>(3);

struct StaticSampler1 {
    D3D12_STATIC_SAMPLER_DESC sampler;
    UINT flags;
};

void Expect(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::uint8_t> BlobBytes(ID3DBlob* blob) {
    const auto* data = static_cast<const std::uint8_t*>(blob->GetBufferPointer());
    return {data, data + blob->GetBufferSize()};
}

class Library {
public:
    Library() : module(LoadLibraryW(L"d3d12_xs.dll")) {
        if (module == nullptr) throw std::runtime_error("d3d12_xs.dll did not load");
        serialize = Resolve<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>("D3D12SerializeVersionedRootSignature");
        createDeserializer = Resolve<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateRootSignatureDeserializer");
    }

    ~Library() { FreeLibrary(module); }

    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;

    HRESULT Serialize(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* description, ID3DBlob** blob, ID3DBlob** errorBlob) const {
        return serialize(description, blob, errorBlob);
    }

    HRESULT CreateDeserializer(const void* sourceData, SIZE_T sourceDataSize, REFIID interfaceId, void** deserializer) const {
        return createDeserializer(sourceData, sourceDataSize, interfaceId, deserializer);
    }

private:
    template<class TFunction>
    TFunction Resolve(const char* name) const {
        const FARPROC address = GetProcAddress(module, name);
        if (address == nullptr) throw std::runtime_error(std::string(name) + " is not exported");
        return reinterpret_cast<TFunction>(reinterpret_cast<void (*)()>(address));
    }

    HMODULE module;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE serialize = nullptr;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER createDeserializer = nullptr;
};

class Signature {
public:
    explicit Signature(D3D_ROOT_SIGNATURE_VERSION version) {
        ranges1[0] = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0};
        ranges1[1] = {D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
        for (UINT index = 0; index < rangeCount; ++index) {
            const D3D12_DESCRIPTOR_RANGE1& range = ranges1[index];
            ranges[index] = {range.RangeType, range.NumDescriptors, range.BaseShaderRegister, range.RegisterSpace, range.OffsetInDescriptorsFromTableStart};
        }

        parameters1[tableParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters1[tableParameter].DescriptorTable = {rangeCount, ranges1};
        parameters1[tableParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        parameters1[constantsParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters1[constantsParameter].Constants = {3, 1, 4};
        parameters1[constantsParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
        parameters1[descriptorParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        parameters1[descriptorParameter].Descriptor = {5, 6, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC};
        parameters1[descriptorParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        parameters[tableParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[tableParameter].DescriptorTable = {rangeCount, ranges};
        parameters[tableParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        parameters[constantsParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[constantsParameter].Constants = {3, 1, 4};
        parameters[constantsParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
        parameters[descriptorParameter].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        parameters[descriptorParameter].Descriptor = {5, 6};
        parameters[descriptorParameter].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        sampler = {D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
            0.5f, 8, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 0.0f, D3D12_FLOAT32_MAX, 7, 1, D3D12_SHADER_VISIBILITY_PIXEL};
        sampler1 = {sampler, 0};

        const D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        expected = {parameterCount, parameters, 1, &sampler, flags};
        description.Version = version;
        switch (version) {
        case D3D_ROOT_SIGNATURE_VERSION_1_0:
            description.Desc_1_0 = expected;
            break;
        case D3D_ROOT_SIGNATURE_VERSION_1_1:
            description.Desc_1_1 = {parameterCount, parameters1, 1, &sampler, flags};
            break;
        default:
            description.Desc_1_1 = {parameterCount, parameters1, 1, &sampler1.sampler, flags};
            break;
        }
    }

    Signature(const Signature&) = delete;
    Signature& operator=(const Signature&) = delete;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC& Description() { return description; }
    const D3D12_ROOT_SIGNATURE_DESC& Expected() const { return expected; }
    D3D12_ROOT_PARAMETER1& Parameter(UINT index) { return parameters1[index]; }
    D3D12_DESCRIPTOR_RANGE1& Range(UINT index) { return ranges1[index]; }
    D3D12_STATIC_SAMPLER_DESC& Sampler() { return sampler; }
    StaticSampler1& Sampler1() { return sampler1; }

private:
    D3D12_DESCRIPTOR_RANGE ranges[rangeCount]{};
    D3D12_DESCRIPTOR_RANGE1 ranges1[rangeCount]{};
    D3D12_ROOT_PARAMETER parameters[parameterCount]{};
    D3D12_ROOT_PARAMETER1 parameters1[parameterCount]{};
    D3D12_STATIC_SAMPLER_DESC sampler{};
    StaticSampler1 sampler1{};
    D3D12_ROOT_SIGNATURE_DESC expected{};
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
};

class SerializedSignature {
public:
    explicit SerializedSignature(ID3DBlob* blob) : bytes(BlobBytes(blob)) {
        Expect(bytes.size() >= 32 && std::memcmp(bytes.data(), "DXBC", 4) == 0, "The host did not produce a DXBC container");
        const std::uint32_t partCount = ReadAbsolute(28);
        for (std::uint32_t index = 0; index < partCount; ++index) {
            const std::size_t part = ReadAbsolute(32 + static_cast<std::size_t>(index) * 4);
            if (std::memcmp(bytes.data() + part, "RTS0", 4) == 0) {
                rootSignature = part + 8;
                return;
            }
        }
        throw std::runtime_error("The host did not produce an RTS0 part");
    }

    std::uint32_t Read(std::size_t offset) const { return ReadAbsolute(rootSignature + offset); }

    void Write(std::size_t offset, std::uint32_t value) {
        Expect(rootSignature + offset + sizeof(value) <= bytes.size(), "A patch fell outside the serialized root signature");
        std::memcpy(bytes.data() + rootSignature + offset, &value, sizeof(value));
    }

    void WriteAbsolute(std::size_t offset, std::uint32_t value) {
        Expect(offset + sizeof(value) <= bytes.size(), "A patch fell outside the container");
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }

    std::size_t Parameter(UINT index) const { return Read(8) + static_cast<std::size_t>(index) * 12; }
    std::size_t Payload(UINT index) const { return Read(Parameter(index) + 8); }
    std::size_t Range(UINT parameter, UINT index) const { return Read(Payload(parameter) + 4) + static_cast<std::size_t>(index) * 24; }
    std::size_t Sampler(UINT index) const { return Read(16) + static_cast<std::size_t>(index) * 52; }
    const std::vector<std::uint8_t>& Bytes() const { return bytes; }

private:
    std::uint32_t ReadAbsolute(std::size_t offset) const {
        Expect(offset + sizeof(std::uint32_t) <= bytes.size(), "A read fell outside the container");
        std::uint32_t value = 0;
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    std::vector<std::uint8_t> bytes;
    std::size_t rootSignature = 0;
};

HRESULT ExpectSerializationMatchesHost(const Library& library, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& description) {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3DBlob> hostBlob;
    ComPtr<ID3DBlob> hostErrorBlob;
    const HRESULT result = library.Serialize(&description, blob.GetAddressOf(), errorBlob.GetAddressOf());
    const HRESULT hostResult = D3D12SerializeVersionedRootSignature(&description, hostBlob.GetAddressOf(), hostErrorBlob.GetAddressOf());
    Expect(result == hostResult, "Serialization returned a different result than the host");
    Expect((blob == nullptr) == (hostBlob == nullptr), "Serialization produced a blob when the host did not, or the reverse");
    Expect((errorBlob == nullptr) == (hostErrorBlob == nullptr), "Serialization produced an error blob when the host did not, or the reverse");
    if (blob != nullptr) Expect(BlobBytes(blob.Get()) == BlobBytes(hostBlob.Get()), "Serialization produced different bytes than the host");
    if (errorBlob != nullptr) Expect(BlobBytes(errorBlob.Get()) == BlobBytes(hostErrorBlob.Get()), "Serialization reported a different error than the host");
    return result;
}

void ExpectDeserializationMatchesHost(const Library& library, const void* sourceData, SIZE_T sourceDataSize) {
    ComPtr<ID3D12RootSignatureDeserializer> deserializer;
    ComPtr<ID3D12RootSignatureDeserializer> hostDeserializer;
    const HRESULT result = library.CreateDeserializer(sourceData, sourceDataSize, IID_PPV_ARGS(&deserializer));
    const HRESULT hostResult = D3D12CreateRootSignatureDeserializer(sourceData, sourceDataSize, IID_PPV_ARGS(&hostDeserializer));
    Expect(result == hostResult, "Deserialization returned a different result than the host");
    Expect((deserializer == nullptr) == (hostDeserializer == nullptr), "Deserialization produced a deserializer when the host did not, or the reverse");
}

void ExpectEqual(const D3D12_ROOT_SIGNATURE_DESC& actual, const D3D12_ROOT_SIGNATURE_DESC& expected) {
    Expect(actual.NumParameters == expected.NumParameters && actual.NumStaticSamplers == expected.NumStaticSamplers && actual.Flags == expected.Flags,
        "The deserialized root signature has a different layout");
    for (UINT index = 0; index < expected.NumParameters; ++index) {
        const D3D12_ROOT_PARAMETER& actualParameter = actual.pParameters[index];
        const D3D12_ROOT_PARAMETER& expectedParameter = expected.pParameters[index];
        Expect(actualParameter.ParameterType == expectedParameter.ParameterType && actualParameter.ShaderVisibility == expectedParameter.ShaderVisibility,
            "A deserialized root parameter has a different type or visibility");
        switch (expectedParameter.ParameterType) {
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            Expect(actualParameter.DescriptorTable.NumDescriptorRanges == expectedParameter.DescriptorTable.NumDescriptorRanges &&
                    std::memcmp(actualParameter.DescriptorTable.pDescriptorRanges, expectedParameter.DescriptorTable.pDescriptorRanges,
                        sizeof(D3D12_DESCRIPTOR_RANGE) * expectedParameter.DescriptorTable.NumDescriptorRanges) == 0,
                "A deserialized descriptor table has different ranges");
            break;
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            Expect(std::memcmp(&actualParameter.Constants, &expectedParameter.Constants, sizeof(expectedParameter.Constants)) == 0,
                "Deserialized root constants are different");
            break;
        default:
            Expect(std::memcmp(&actualParameter.Descriptor, &expectedParameter.Descriptor, sizeof(expectedParameter.Descriptor)) == 0,
                "A deserialized root descriptor is different");
            break;
        }
    }
    Expect(std::memcmp(actual.pStaticSamplers, expected.pStaticSamplers, sizeof(D3D12_STATIC_SAMPLER_DESC) * expected.NumStaticSamplers) == 0,
        "Deserialized static samplers are different");
}

template<class TChange>
void ExpectUnknownDescriptionUnimplemented(const Library& library, D3D_ROOT_SIGNATURE_VERSION version, TChange&& change) {
    Signature signature(version);
    change(signature);
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;
    AnyBox::Tests::ExpectUnimplemented("D3D12SerializeVersionedRootSignature",
        [&] { library.Serialize(&signature.Description(), blob.GetAddressOf(), errorBlob.GetAddressOf()); });
    Expect(blob == nullptr && errorBlob == nullptr, "An unimplemented serialization produced output");
}

void ExpectUnknownSerializedUnimplemented(const Library& library, const SerializedSignature& original, std::size_t offset, std::uint32_t expected, std::uint32_t patched) {
    SerializedSignature signature = original;
    Expect(signature.Read(offset) == expected, "The serialized root signature does not hold the value being patched");
    signature.Write(offset, patched);
    ComPtr<ID3D12RootSignatureDeserializer> deserializer;
    AnyBox::Tests::ExpectUnimplemented("D3D12CreateRootSignatureDeserializer",
        [&] { library.CreateDeserializer(signature.Bytes().data(), signature.Bytes().size(), IID_PPV_ARGS(&deserializer)); });
    Expect(deserializer == nullptr, "An unimplemented deserialization produced output");
}

void TestSerializationMatchesHost(const Library& library) {
    for (const D3D_ROOT_SIGNATURE_VERSION version : {D3D_ROOT_SIGNATURE_VERSION_1_0, D3D_ROOT_SIGNATURE_VERSION_1_1, rootSignatureVersion1_2}) {
        Signature signature(version);
        const HRESULT result = ExpectSerializationMatchesHost(library, signature.Description());
        Expect(version == rootSignatureVersion1_2 || SUCCEEDED(result), "A valid root signature failed to serialize");
    }

    Signature mixedTable(D3D_ROOT_SIGNATURE_VERSION_1_1);
    mixedTable.Range(1).RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    Expect(FAILED(ExpectSerializationMatchesHost(library, mixedTable.Description())), "A table mixing samplers and views was accepted");

    Signature conflictingFlags(D3D_ROOT_SIGNATURE_VERSION_1_1);
    conflictingFlags.Range(0).Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    Expect(FAILED(ExpectSerializationMatchesHost(library, conflictingFlags.Description())), "Conflicting range flags were accepted");
}

void TestRoundTrip(const Library& library) {
    for (const D3D_ROOT_SIGNATURE_VERSION version : {D3D_ROOT_SIGNATURE_VERSION_1_0, D3D_ROOT_SIGNATURE_VERSION_1_1, rootSignatureVersion1_2}) {
        Signature signature(version);
        ComPtr<ID3DBlob> blob;
        if (FAILED(library.Serialize(&signature.Description(), blob.GetAddressOf(), nullptr))) {
            Expect(version == rootSignatureVersion1_2, "A valid root signature failed to serialize");
            continue;
        }
        ComPtr<ID3D12RootSignatureDeserializer> deserializer;
        Expect(SUCCEEDED(library.CreateDeserializer(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&deserializer))),
            "A serialized root signature failed to deserialize");
        ExpectEqual(*deserializer->GetRootSignatureDesc(), signature.Expected());
    }
}

void TestUnknownDescriptionValues(const Library& library) {
    const auto unknownRootFlag = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0x40000000);
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_0, [&](Signature& signature) { signature.Description().Desc_1_0.Flags |= unknownRootFlag; });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1, [&](Signature& signature) { signature.Description().Desc_1_1.Flags |= unknownRootFlag; });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Description().Version = static_cast<D3D_ROOT_SIGNATURE_VERSION>(4); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Parameter(constantsParameter).ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(5); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Parameter(constantsParameter).ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(8); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Parameter(descriptorParameter).Descriptor.Flags = static_cast<D3D12_ROOT_DESCRIPTOR_FLAGS>(0x1); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Range(0).RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(4); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Range(1).Flags = static_cast<D3D12_DESCRIPTOR_RANGE_FLAGS>(0x20); });
    ExpectUnknownDescriptionUnimplemented(library, D3D_ROOT_SIGNATURE_VERSION_1_1,
        [](Signature& signature) { signature.Sampler().ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(8); });
    ExpectUnknownDescriptionUnimplemented(library, rootSignatureVersion1_2,
        [](Signature& signature) { signature.Sampler1().flags = 0x4; });
}

void TestUnknownSerializedValues(const Library& library) {
    Signature signature(D3D_ROOT_SIGNATURE_VERSION_1_1);
    ComPtr<ID3DBlob> blob;
    Expect(SUCCEEDED(D3D12SerializeVersionedRootSignature(&signature.Description(), blob.GetAddressOf(), nullptr)), "The host failed to serialize a valid root signature");
    const SerializedSignature original(blob.Get());

    ComPtr<ID3D12RootSignatureDeserializer> deserializer;
    Expect(SUCCEEDED(library.CreateDeserializer(original.Bytes().data(), original.Bytes().size(), IID_PPV_ARGS(&deserializer))),
        "A root signature serialized by the host failed to deserialize");

    ExpectUnknownSerializedUnimplemented(library, original, 0, D3D_ROOT_SIGNATURE_VERSION_1_1, 4);
    ExpectUnknownSerializedUnimplemented(library, original, 20, 0x5, 0x40000005);
    ExpectUnknownSerializedUnimplemented(library, original, original.Parameter(tableParameter), D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, 5);
    ExpectUnknownSerializedUnimplemented(library, original, original.Parameter(constantsParameter) + 4, D3D12_SHADER_VISIBILITY_GEOMETRY, 8);
    ExpectUnknownSerializedUnimplemented(library, original, original.Payload(descriptorParameter) + 8, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, 0x9);
    ExpectUnknownSerializedUnimplemented(library, original, original.Range(tableParameter, 0), D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4);
    ExpectUnknownSerializedUnimplemented(library, original, original.Range(tableParameter, 1) + 16, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0x21);
    ExpectUnknownSerializedUnimplemented(library, original, original.Sampler(0) + 48, D3D12_SHADER_VISIBILITY_PIXEL, 8);
}

void TestMalformedDataMatchesHost(const Library& library) {
    Signature signature(D3D_ROOT_SIGNATURE_VERSION_1_1);
    ComPtr<ID3DBlob> blob;
    Expect(SUCCEEDED(D3D12SerializeVersionedRootSignature(&signature.Description(), blob.GetAddressOf(), nullptr)), "The host failed to serialize a valid root signature");
    const SerializedSignature original(blob.Get());
    const std::vector<std::uint8_t>& valid = original.Bytes();

    ExpectDeserializationMatchesHost(library, valid.data(), valid.size());
    ExpectDeserializationMatchesHost(library, nullptr, 0);
    ExpectDeserializationMatchesHost(library, "DXBC", 4);
    const std::vector<std::uint8_t> filler(64, 0xCD);
    ExpectDeserializationMatchesHost(library, filler.data(), filler.size());
    ExpectDeserializationMatchesHost(library, valid.data(), valid.size() / 2);

    std::vector<std::uint8_t> corruptedHash = valid;
    corruptedHash[4] = static_cast<std::uint8_t>(~corruptedHash[4]);
    ExpectDeserializationMatchesHost(library, corruptedHash.data(), corruptedHash.size());

    SerializedSignature manyParts = original;
    manyParts.WriteAbsolute(28, 0xFFFFFFFF);
    ExpectDeserializationMatchesHost(library, manyParts.Bytes().data(), manyParts.Bytes().size());
}

bool HasHostUnknownSerializedValues(const SerializedSignature& signature) {
    return AnyBox::D3D12Xs::HasHostUnknownSerializedValues(signature.Bytes().data(), signature.Bytes().size());
}

void TestOversizedCountsAreLeftToHost() {
    Signature signature(D3D_ROOT_SIGNATURE_VERSION_1_1);
    ComPtr<ID3DBlob> blob;
    Expect(SUCCEEDED(D3D12SerializeVersionedRootSignature(&signature.Description(), blob.GetAddressOf(), nullptr)), "The host failed to serialize a valid root signature");
    const SerializedSignature original(blob.Get());
    Expect(!HasHostUnknownSerializedValues(original), "A valid root signature was reported as unknown");

    SerializedSignature manyParameters = original;
    manyParameters.Write(4, 0xFFFFFFFF);
    Expect(!HasHostUnknownSerializedValues(manyParameters), "An oversized parameter count was reported as unknown");

    SerializedSignature manyRanges = original;
    manyRanges.Write(original.Payload(tableParameter), 0xFFFFFFFF);
    Expect(!HasHostUnknownSerializedValues(manyRanges), "An oversized range count was reported as unknown");

    SerializedSignature manySamplers = original;
    manySamplers.Write(12, 0xFFFFFFFF);
    Expect(!HasHostUnknownSerializedValues(manySamplers), "An oversized sampler count was reported as unknown");

    SerializedSignature distantParameters = original;
    distantParameters.Write(8, 0xFFFFFFF0);
    Expect(!HasHostUnknownSerializedValues(distantParameters), "A parameter offset outside the part was reported as unknown");

    SerializedSignature unknownFlagsWithManyParameters = manyParameters;
    unknownFlagsWithManyParameters.Write(20, 0x40000005);
    Expect(HasHostUnknownSerializedValues(unknownFlagsWithManyParameters), "Unknown flags were missed in a root signature with an oversized parameter count");
}

}

int main() {
    return AnyBox::Tests::Run([] {
        const Library library;
        TestSerializationMatchesHost(library);
        TestRoundTrip(library);
        TestUnknownDescriptionValues(library);
        TestUnknownSerializedValues(library);
        TestMalformedDataMatchesHost(library);
        TestOversizedCountsAreLeftToHost();
    });
}
