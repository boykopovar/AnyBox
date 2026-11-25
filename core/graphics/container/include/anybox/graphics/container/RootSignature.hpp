#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>

namespace AnyBox::Graphics::Container {
enum class RootParameterKind : std::uint32_t { Table, Constants, ConstantBuffer, ShaderResource, UnorderedAccess };
enum class RootRangeKind : std::uint32_t { ShaderResource, UnorderedAccess, ConstantBuffer, Sampler };

struct RootRange {
    RootRangeKind kind;
    std::uint32_t count;
    std::uint32_t firstRegister;
    std::uint32_t space;
    std::uint32_t flags;
    std::uint32_t tableOffset;
    bool operator==(const RootRange&) const = default;
};

struct RootParameter {
    RootParameterKind kind;
    std::uint32_t visibility;
    std::uint32_t shaderRegister = 0;
    std::uint32_t space = 0;
    std::uint32_t wordCount = 0;
    std::uint32_t flags = 0;
    std::vector<RootRange> ranges;
    bool operator==(const RootParameter&) const = default;
};

struct RootSignatureLayout {
    std::uint32_t version;
    std::uint32_t flags;
    std::uint32_t staticSamplerCount;
    std::vector<RootParameter> parameters;
    bool operator==(const RootSignatureLayout&) const = default;
};

RootSignatureLayout ReadRootSignatureLayout(std::span<const std::byte> container);
}
