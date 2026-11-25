#pragma once

#include <anybox/graphics/commands/RootState.hpp>
#include <anybox/graphics/container/NativeBindings.hpp>

namespace AnyBox::Shader {
class NativeRootArgumentWriter {
public:
    NativeRootArgumentWriter(const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> registers);
    void Write(const Graphics::Commands::RootState& state, std::span<std::uint32_t> destination) const;
    std::uint32_t GetWordCount() const noexcept { return static_cast<std::uint32_t>(copies.size()); }

private:
    struct Copy {
        Graphics::Container::RootParameterKind kind;
        std::uint32_t parameter;
        std::uint32_t word;
        std::uint32_t wordCount;
        bool operator==(const Copy&) const = default;
    };
    std::vector<Copy> copies;
};
}
