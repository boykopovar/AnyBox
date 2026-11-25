#include "State.hpp"
#include <anybox/graphics/container/SerializedPixelInterface.hpp>

AnyBox::Graphics::Container::SerializedPixelInterface AnyBox::Graphics::Container::ReadSerializedPixelInterface(std::span<const std::byte> state) {
    auto context = SerializedGraphics::ReadRegisters(state).subspan(60);
    auto hashes = SerializedGraphics::ReadHashes(state);
    auto inputAddress = ReadInteger(context, 0);
    auto control = ReadInteger(context, 4);
    auto count = control & 63;
    if ((inputAddress & ~65535u) != 0 || count > 32) throw std::invalid_argument("Xbox serialized pixel input address or interpolator count is invalid");
    if ((control & ~0xC13Fu) != 0) throw std::runtime_error("Xbox serialized pixel parameter generation, primitive and offchip interpolation are not implemented");
    if (hashes.size() != 68 + static_cast<std::size_t>(count) * 4) throw std::invalid_argument("Xbox serialized pixel interpolation table disagrees with its register count");
    SerializedPixelInterface result{(control & 0x8000) != 0 ? 32u : 64u, inputAddress, count, ReadInteger(context, 8), ReadInteger(context, 16), ReadInteger(context, 24), {}};
    for (std::uint32_t index = 0; index < count; ++index) result.interpolators[index] = ReadInteger(hashes, 56 + index * 4);
    return result;
}
