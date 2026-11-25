#include "State.hpp"
#include <algorithm>

namespace {
std::uint32_t BlendOperation(std::uint32_t value) {
    constexpr std::array<std::uint32_t, 5> encodings{0, 1, 4, 2, 3};
    auto found = std::ranges::find(encodings, value);
    if (found == encodings.end()) throw std::runtime_error("Xbox serialized blend operation is not implemented");
    return static_cast<std::uint32_t>(found - encodings.begin()) + 1;
}
}

std::array<AnyBox::Graphics::Container::SerializedBlendTarget, 8> AnyBox::Graphics::Container::SerializedGraphics::ReadBlend(std::span<const std::byte> context, std::span<const std::byte> auxiliary) {
    std::array<SerializedBlendTarget, 8> result{};
    auto masks = ReadInteger(context, 116);
    auto logic = Byte(auxiliary, 43);
    for (std::size_t index = 0; index < result.size(); ++index) {
        auto control = ReadInteger(context, 120 + index * 4);
        auto& target = result[index];
        target.enable = (control & 0x40000000) != 0;
        target.logicEnable = (logic & (1u << index)) != 0;
        target.logicOperation = Byte(auxiliary, 45 + index);
        target.writeMask = static_cast<std::uint8_t>((masks >> (index * 4)) & 15);
        if (target.logicOperation > 15) throw std::runtime_error("Xbox serialized logic operation is not implemented");
        if (target.enable && target.logicEnable) throw std::invalid_argument("Xbox serialized blend and logic operations are both enabled");
        if (!target.enable) continue;
        target.source = (control & 31) + 1;
        target.destination = ((control >> 8) & 31) + 1;
        target.sourceAlpha = ((control >> 16) & 31) + 1;
        target.destinationAlpha = ((control >> 24) & 31) + 1;
        if (target.source > 19 || target.destination > 19 || target.sourceAlpha > 19 || target.destinationAlpha > 19) throw std::runtime_error("Xbox serialized extended blend factor is not implemented");
        target.operation = BlendOperation((control >> 5) & 7);
        target.alphaOperation = BlendOperation((control >> 21) & 7);
    }
    return result;
}
