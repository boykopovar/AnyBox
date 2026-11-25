#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyBox::Graphics::Container {
enum class NativeRootKind : std::uint32_t { Constants = 0, Descriptor = 1, Table = 3, ImmediateSampler = 4, ResourceHeap = 5, SamplerHeap = 6 };
enum class NativeResourceKind : std::uint8_t { ShaderResource, Sampler, ConstantBuffer, UnorderedAccess };

struct NativeRootBinding {
    NativeRootKind kind;
    std::uint32_t tableOffset;
    std::uint32_t registerCount;
    std::uint32_t shaderRegister;
    std::uint8_t firstUserRegister;
    std::uint8_t rootParameter;
    bool operator==(const NativeRootBinding&) const = default;
};

struct NativeResourceBinding {
    NativeResourceKind kind;
    NativeRootKind rootKind;
    std::uint32_t tableOffset;
    std::uint32_t firstShaderRegister;
    std::uint32_t stride;
    std::uint8_t descriptorWords;
    std::uint8_t firstUserRegister;
    std::uint8_t rootParameter;
    bool operator==(const NativeResourceBinding&) const = default;
};

struct NativeBindings {
    std::vector<NativeRootBinding> roots;
    std::vector<NativeResourceBinding> resources;
};

NativeBindings ReadNativeComputeBindings(std::span<const std::byte> program);
NativeBindings ReadNativeBindings(std::span<const std::byte> program);
NativeResourceBinding ReadNativeResourceBinding(std::span<const std::byte> data);
}
