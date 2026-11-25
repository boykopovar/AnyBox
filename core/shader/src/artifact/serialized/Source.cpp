#include <anybox/shader/SerializedComputeSource.hpp>
#include <anybox/graphics/container/View.hpp>
#include <algorithm>
#include <stdexcept>

namespace {
void Word(std::span<std::byte> output, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) output[index] = static_cast<std::byte>(value >> (index * 8));
}
}

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashSerializedComputeSource(const SerializedComputeSource& source, const ShaderDigest& root, std::uint32_t subgroupSize) {
    if (subgroupSize != 32 && subgroupSize != 64) throw std::invalid_argument("Serialized compute source subgroup size is invalid");
    constexpr std::array<std::byte, 8> domain{std::byte{'A'}, std::byte{'B'}, std::byte{'X'}, std::byte{'S'}, std::byte{'C'}, std::byte{'0'}, std::byte{'0'}, std::byte{'1'}};
    std::array<std::byte, 48> header{};
    std::ranges::copy(domain, header.begin());
    std::ranges::copy(root, header.begin() + 8);
    Word(std::span(header).subspan(40, 4), subgroupSize);
    Word(std::span(header).subspan(44, 4), 1);
    const std::array<std::uint32_t, 4> kinds{0, 1, 6, 7};
    const std::array frames{source.dispatchState, source.auxiliaryState, source.code, source.metadata};
    std::array<std::array<std::byte, 8>, 4> descriptions{};
    std::array<std::span<const std::byte>, 9> parts{};
    parts[0] = header;
    for (std::size_t index = 0; index < frames.size(); ++index) {
        if (frames[index].size() > UINT32_MAX) throw std::length_error("Serialized compute source part exceeds its size field");
        Word(std::span(descriptions[index]).first(4), kinds[index]);
        Word(std::span(descriptions[index]).last(4), static_cast<std::uint32_t>(frames[index].size()));
        parts[index * 2 + 1] = descriptions[index];
        parts[index * 2 + 2] = frames[index];
    }
    return HashShaderParts(parts);
}

AnyBox::Shader::ShaderDigest AnyBox::Shader::HashSerializedRootSignature(std::span<const std::byte> data) {
    Graphics::Container::DxbcView container(data);
    auto root = container.FindPart(0x30535452);
    if (!root || root->data.size() < 24) throw std::invalid_argument("Serialized root signature part is missing or truncated");
    return HashShader(root->data);
}
