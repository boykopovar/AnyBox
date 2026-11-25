#include "State.hpp"
#include <cmath>

AnyBox::Graphics::Container::SerializedRasterizerState AnyBox::Graphics::Container::SerializedGraphics::ReadRasterizer(std::span<const std::byte> context, std::span<const std::byte> auxiliary) {
    auto control = ReadInteger(context, 72);
    auto fill = ((control >> 8) & 7) + 1;
    auto cull = control & 3;
    if (fill != 2 && fill != 3) throw std::runtime_error("Xbox serialized polygon fill mode is not implemented");
    if (cull == 3) throw std::runtime_error("Xbox serialized simultaneous front and back culling is not implemented");
    auto bias = Float(context, 92);
    auto clamp = Float(context, 84);
    auto slope = Float(context, 88) * 0.0625f;
    if (!std::isfinite(bias) || !std::isfinite(clamp) || !std::isfinite(slope)) throw std::invalid_argument("Xbox serialized depth bias is not finite");
    if (static_cast<double>(bias) < -2147483648.0 || static_cast<double>(bias) >= 2147483648.0) throw std::out_of_range("Xbox serialized depth bias exceeds its integer representation");
    auto flags = Byte(auxiliary, 44);
    return {fill, cull == 0 ? 1u : cull + 1, (control & 4) == 0, static_cast<std::int32_t>(bias), clamp, slope, (ReadInteger(context, 68) & 0x4000000) == 0, (flags & 2) != 0, (flags & 4) != 0, (flags >> 3) & 1u, Byte(auxiliary, 42) & 15u};
}
