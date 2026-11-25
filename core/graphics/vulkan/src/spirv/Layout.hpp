#pragma once

#include <anybox/vulkan/Spirv.hpp>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace AnyBox::Vulkan::Spirv {
using Words = std::span<const std::uint32_t>;
struct Member {
    std::optional<std::uint32_t> offset;
    std::optional<std::uint32_t> matrixStride;
    bool rowMajor = false;
};

class Layout {
public:
    explicit Layout(Words code);
    VkPushConstantRange Read(std::string_view entryPoint);

private:
    std::uint64_t size(std::uint32_t id, const Member& layout);
    std::uint64_t constant(std::uint32_t id) const;
    Words type(std::uint32_t id) const;
    Words code;
    std::unordered_map<std::uint32_t, Words> types;
    std::unordered_map<std::uint32_t, Words> constants;
    std::unordered_map<std::uint32_t, std::uint32_t> strides;
    std::unordered_map<std::uint64_t, Member> members;
    std::unordered_set<std::uint32_t> active;
    std::unordered_set<std::uint32_t> blocks;
};
}
