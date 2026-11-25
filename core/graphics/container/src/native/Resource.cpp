#include <anybox/graphics/container/NativeBindings.hpp>
#include "../Integer.hpp"

AnyBox::Graphics::Container::NativeResourceBinding AnyBox::Graphics::Container::ReadNativeResourceBinding(std::span<const std::byte> data) {
    if (data.size() != 20) throw std::invalid_argument("Xbox native resource binding size is invalid");
    auto byte = [&](std::size_t index) { return std::to_integer<std::uint8_t>(data[index]); };
    if (byte(0) > 3) throw std::runtime_error("Xbox native resource binding kind is not implemented");
    auto root = byte(1);
    if (root != 0 && root != 1 && root != 3 && root != 4 && root != 5 && root != 6) throw std::runtime_error("Xbox native root binding kind is not implemented");
    if (byte(2) != 0 || byte(3) != 0 || byte(19) != 0) throw std::runtime_error("Xbox native resource binding flags are not implemented");
    return {static_cast<NativeResourceKind>(byte(0)), static_cast<NativeRootKind>(root), ReadInteger(data, 4), ReadInteger(data, 8), ReadInteger(data, 12), byte(16), byte(17), byte(18)};
}
