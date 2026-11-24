#include "RootSignature.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#else
#error Root signature forwarding requires Windows
#endif

namespace {

template<class... TFlags>
constexpr std::uint32_t Bits(TFlags... flags) {
    return (static_cast<std::uint32_t>(flags) | ...);
}

constexpr std::uint32_t knownRootSignatureFlags = Bits(
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT,
    D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS,
    D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED,
    D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);

constexpr std::uint32_t knownRangeFlags = Bits(
    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS);

constexpr std::uint32_t knownDescriptorFlags = Bits(
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

constexpr std::uint32_t rootSignatureVersion1_2 = 3;
constexpr std::uint32_t samplerFlagUintBorderColor = 0x1;
constexpr std::uint32_t samplerFlagNonNormalizedCoordinates = 0x2;
constexpr std::uint32_t knownSamplerFlags = samplerFlagUintBorderColor | samplerFlagNonNormalizedCoordinates;

struct StaticSampler1 {
    D3D12_STATIC_SAMPLER_DESC sampler;
    std::uint32_t flags;
};

constexpr std::size_t serializedParameterSize = 12;
constexpr std::size_t serializedSamplerSize = 52;
constexpr std::size_t serializedSampler1Size = 56;

bool KnownParameterType(std::uint32_t type) {
    return type <= D3D12_ROOT_PARAMETER_TYPE_UAV;
}

bool KnownVisibility(std::uint32_t visibility) {
    return visibility <= D3D12_SHADER_VISIBILITY_MESH;
}

bool KnownRangeType(std::uint32_t type) {
    return type <= D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
}

bool IsRootDescriptor(std::uint32_t type) {
    return type == D3D12_ROOT_PARAMETER_TYPE_CBV || type == D3D12_ROOT_PARAMETER_TYPE_SRV || type == D3D12_ROOT_PARAMETER_TYPE_UAV;
}

bool UnknownParameter(const D3D12_ROOT_PARAMETER& parameter) {
    if (!KnownParameterType(parameter.ParameterType) || !KnownVisibility(parameter.ShaderVisibility)) return true;
    if (parameter.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE || parameter.DescriptorTable.pDescriptorRanges == nullptr) return false;
    for (UINT index = 0; index < parameter.DescriptorTable.NumDescriptorRanges; ++index) {
        if (!KnownRangeType(parameter.DescriptorTable.pDescriptorRanges[index].RangeType)) return true;
    }
    return false;
}

bool UnknownParameter(const D3D12_ROOT_PARAMETER1& parameter) {
    if (!KnownParameterType(parameter.ParameterType) || !KnownVisibility(parameter.ShaderVisibility)) return true;
    if (IsRootDescriptor(parameter.ParameterType)) return (parameter.Descriptor.Flags & ~knownDescriptorFlags) != 0;
    if (parameter.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE || parameter.DescriptorTable.pDescriptorRanges == nullptr) return false;
    for (UINT index = 0; index < parameter.DescriptorTable.NumDescriptorRanges; ++index) {
        const D3D12_DESCRIPTOR_RANGE1& range = parameter.DescriptorTable.pDescriptorRanges[index];
        if (!KnownRangeType(range.RangeType) || (range.Flags & ~knownRangeFlags) != 0) return true;
    }
    return false;
}

bool UnknownSampler(const D3D12_STATIC_SAMPLER_DESC& sampler) {
    return !KnownVisibility(sampler.ShaderVisibility);
}

bool UnknownSampler(const StaticSampler1& sampler) {
    return UnknownSampler(sampler.sampler) || (sampler.flags & ~knownSamplerFlags) != 0;
}

template<class TSampler, class TDescription>
bool UnknownDescription(const TDescription& description) {
    if ((description.Flags & ~knownRootSignatureFlags) != 0) return true;
    if (description.pParameters != nullptr) {
        for (UINT index = 0; index < description.NumParameters; ++index) {
            if (UnknownParameter(description.pParameters[index])) return true;
        }
    }
    if (description.pStaticSamplers != nullptr) {
        const auto* samplers = reinterpret_cast<const TSampler*>(description.pStaticSamplers);
        for (UINT index = 0; index < description.NumStaticSamplers; ++index) {
            if (UnknownSampler(samplers[index])) return true;
        }
    }
    return false;
}

class SerializedPart {
public:
    SerializedPart(const std::uint8_t* data, std::size_t size) : data(data), size(size) {}

    bool Read(std::size_t offset, std::uint32_t& value) const {
        if (offset > size || size - offset < sizeof(value)) return false;
        std::memcpy(&value, data + offset, sizeof(value));
        return true;
    }

    bool Holds(std::size_t offset, std::uint32_t count, std::size_t stride) const {
        return count == 0 || (offset <= size && count <= (size - offset) / stride);
    }

private:
    const std::uint8_t* data;
    std::size_t size;
};

bool UnknownSerializedParameter(const SerializedPart& part, std::uint32_t version, std::size_t offset) {
    std::uint32_t type = 0;
    std::uint32_t visibility = 0;
    std::uint32_t payloadOffset = 0;
    if (!part.Read(offset, type) || !part.Read(offset + 4, visibility) || !part.Read(offset + 8, payloadOffset)) return false;
    if (!KnownParameterType(type) || !KnownVisibility(visibility)) return true;
    const std::size_t payload = payloadOffset;
    std::uint32_t flags = 0;
    if (IsRootDescriptor(type)) return version >= D3D_ROOT_SIGNATURE_VERSION_1_1 && part.Read(payload + 8, flags) && (flags & ~knownDescriptorFlags) != 0;
    if (type != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) return false;
    std::uint32_t rangeCount = 0;
    std::uint32_t rangesOffset = 0;
    if (!part.Read(payload, rangeCount) || !part.Read(payload + 4, rangesOffset)) return false;
    const std::size_t rangeSize = version >= D3D_ROOT_SIGNATURE_VERSION_1_1 ? 24 : 20;
    if (!part.Holds(rangesOffset, rangeCount, rangeSize)) return false;
    for (std::uint32_t index = 0; index < rangeCount; ++index) {
        const std::size_t range = rangesOffset + static_cast<std::size_t>(index) * rangeSize;
        std::uint32_t rangeType = 0;
        if (!part.Read(range, rangeType)) return false;
        if (!KnownRangeType(rangeType)) return true;
        if (version >= D3D_ROOT_SIGNATURE_VERSION_1_1 && part.Read(range + 16, flags) && (flags & ~knownRangeFlags) != 0) return true;
    }
    return false;
}

bool UnknownSerializedRootSignature(const SerializedPart& part) {
    std::uint32_t version = 0;
    std::uint32_t parameterCount = 0;
    std::uint32_t parametersOffset = 0;
    std::uint32_t samplerCount = 0;
    std::uint32_t samplersOffset = 0;
    std::uint32_t flags = 0;
    if (!part.Read(0, version) || !part.Read(4, parameterCount) || !part.Read(8, parametersOffset) || !part.Read(12, samplerCount) ||
        !part.Read(16, samplersOffset) || !part.Read(20, flags)) {
        return false;
    }
    if (version < D3D_ROOT_SIGNATURE_VERSION_1_0 || version > rootSignatureVersion1_2) return true;
    if ((flags & ~knownRootSignatureFlags) != 0) return true;
    if (!part.Holds(parametersOffset, parameterCount, serializedParameterSize)) return false;
    for (std::uint32_t index = 0; index < parameterCount; ++index) {
        if (UnknownSerializedParameter(part, version, parametersOffset + static_cast<std::size_t>(index) * serializedParameterSize)) return true;
    }
    const std::size_t samplerSize = version == rootSignatureVersion1_2 ? serializedSampler1Size : serializedSamplerSize;
    if (!part.Holds(samplersOffset, samplerCount, samplerSize)) return false;
    for (std::uint32_t index = 0; index < samplerCount; ++index) {
        const std::size_t sampler = samplersOffset + static_cast<std::size_t>(index) * samplerSize;
        std::uint32_t visibility = 0;
        if (!part.Read(sampler + 48, visibility)) return false;
        if (!KnownVisibility(visibility)) return true;
        std::uint32_t samplerFlags = 0;
        if (version == rootSignatureVersion1_2 && part.Read(sampler + 52, samplerFlags) && (samplerFlags & ~knownSamplerFlags) != 0) return true;
    }
    return false;
}

template<class TFunction>
TFunction HostFunction(const char* name) {
    const HMODULE module = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    const FARPROC address = module == nullptr ? nullptr : GetProcAddress(module, name);
    if (address == nullptr) throw std::runtime_error(std::string("d3d12.dll!") + name + " is not available on this host");
    return reinterpret_cast<TFunction>(reinterpret_cast<void (*)()>(address));
}

}

bool AnyBox::D3D12Xs::HasHostUnknownValues(const void* description) {
    if (description == nullptr) return false;
    const auto& versioned = *static_cast<const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(description);
    switch (static_cast<std::uint32_t>(versioned.Version)) {
    case D3D_ROOT_SIGNATURE_VERSION_1_0:
        return UnknownDescription<D3D12_STATIC_SAMPLER_DESC>(versioned.Desc_1_0);
    case D3D_ROOT_SIGNATURE_VERSION_1_1:
        return UnknownDescription<D3D12_STATIC_SAMPLER_DESC>(versioned.Desc_1_1);
    case rootSignatureVersion1_2:
        return UnknownDescription<StaticSampler1>(versioned.Desc_1_1);
    default:
        return true;
    }
}

bool AnyBox::D3D12Xs::HasHostUnknownSerializedValues(const void* sourceData, std::size_t sourceDataSize) {
    constexpr std::size_t headerSize = 32;
    if (sourceData == nullptr || sourceDataSize < headerSize) return false;
    const auto* bytes = static_cast<const std::uint8_t*>(sourceData);
    if (std::memcmp(bytes, "DXBC", 4) != 0) return false;
    const SerializedPart container(bytes, sourceDataSize);
    std::uint32_t partCount = 0;
    if (!container.Read(28, partCount)) return false;
    for (std::uint32_t index = 0; index < partCount; ++index) {
        std::uint32_t partOffset = 0;
        std::uint32_t fourCc = 0;
        std::uint32_t partSize = 0;
        if (!container.Read(headerSize + static_cast<std::size_t>(index) * 4, partOffset) || !container.Read(partOffset, fourCc) ||
            !container.Read(static_cast<std::size_t>(partOffset) + 4, partSize)) {
            return false;
        }
        if (std::memcmp(&fourCc, "RTS0", 4) != 0) continue;
        const std::size_t partStart = static_cast<std::size_t>(partOffset) + 8;
        if (partSize > sourceDataSize - partStart) return false;
        return UnknownSerializedRootSignature(SerializedPart(bytes + partStart, partSize));
    }
    return false;
}

std::int32_t AnyBox::D3D12Xs::SerializeVersionedRootSignature(const void* description, void** blob, void** errorBlob) {
    static const auto serialize = HostFunction<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>("D3D12SerializeVersionedRootSignature");
    return serialize(
        static_cast<const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(description),
        reinterpret_cast<ID3DBlob**>(blob),
        reinterpret_cast<ID3DBlob**>(errorBlob));
}

std::int32_t AnyBox::D3D12Xs::CreateRootSignatureDeserializer(const void* sourceData, std::size_t sourceDataSize, const void* interfaceId, void** deserializer) {
    static const auto createDeserializer = HostFunction<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateRootSignatureDeserializer");
    return createDeserializer(sourceData, sourceDataSize, *static_cast<const IID*>(interfaceId), deserializer);
}
