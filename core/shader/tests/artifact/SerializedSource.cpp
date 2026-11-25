#include <anybox/shader/SerializedComputeSource.hpp>
#include <anybox/graphics/container/View.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("Serialized compute identity test failed");
}

template<class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const std::exception&) { caught = true; }
    Require(caught);
}
}

void CheckSerializedSourceIdentity() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    std::array<std::byte, 8> bytes{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}};
    auto data = std::span<const std::byte>(bytes);
    SerializedComputeSource source{data.first(2), data.subspan(2, 1), data.subspan(3, 3), data.last(2)};
    ShaderDigest root{};
    auto expected = HashSerializedComputeSource(source, root, 32);
    Require(DigestName(expected) == "879d7b1182499e3ba147070c19df6dc29fabef753ad9f238e2bd2ee064547b65");
    auto copy = bytes;
    auto copied = std::span<const std::byte>(copy);
    Require(HashSerializedComputeSource({copied.first(2), copied.subspan(2, 1), copied.subspan(3, 3), copied.last(2)}, root, 32) == expected);
    Require(HashSerializedComputeSource(source, root, 64) != expected);
    Reject([&] { (void)HashSerializedComputeSource(source, root, 16); });
    auto differentRoot = root;
    differentRoot[31] = std::byte{1};
    Require(HashSerializedComputeSource(source, differentRoot, 32) != expected);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] ^= std::byte{1};
        Require(HashSerializedComputeSource(source, root, 32) != expected);
        bytes[index] ^= std::byte{1};
    }
    auto reframed = source;
    reframed.dispatchState = data.first(1);
    reframed.auxiliaryState = data.subspan(1, 2);
    Require(HashSerializedComputeSource(reframed, root, 32) != expected);
    const std::array parts{data.first(3), data.subspan(3, 2), std::span<const std::byte>{}, data.last(3)};
    Require(HashShaderParts(parts) == HashShader(data));
    Require(HashShaderParts({}) == HashShader({}));
    std::array<std::uint32_t, 6> words{2, 0, 24, 0, 24, 0};
    auto signature = std::as_bytes(std::span(words));
    auto binary = BuildDxbc(std::array<Part, 1>{{{0x30535452, signature}}});
    auto hash = HashSerializedRootSignature(binary);
    Require(hash == HashShader(signature));
    auto extra = BuildDxbc(std::array<Part, 2>{{{0x12345678, data}, {0x30535452, signature}}});
    Require(HashSerializedRootSignature(extra) == hash);
    auto duplicate = BuildDxbc(std::array<Part, 2>{{{0x30535452, signature}, {0x30535452, signature}}});
    Reject([&] { (void)HashSerializedRootSignature(duplicate); });
    Reject([&] { (void)HashSerializedRootSignature(BuildDxbc({})); });
    Reject([&] { (void)HashSerializedRootSignature(BuildDxbc(std::array<Part, 1>{{{0x30535452, data}}})); });
    binary.back() ^= std::byte{1};
    Reject([&] { (void)HashSerializedRootSignature(binary); });
}
