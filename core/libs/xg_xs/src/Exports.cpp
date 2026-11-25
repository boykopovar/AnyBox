#include <xg_xs.hpp>
#include <anybox/layout/Texture.hpp>
#include <stdexcept>

std::int32_t XGSuggestSwizzleMode(const XGTextureDescription* description, std::uint32_t flags, std::uint32_t* swizzleMode) noexcept(false) {
    if (!description || !swizzleMode) throw std::invalid_argument("Texture swizzle suggestion arguments are null");
    if (flags != 0) throw std::runtime_error("Texture swizzle suggestion flags are not implemented");
    auto selected = AnyBox::Layout::SuggestTextureLayout(*description);
    *swizzleMode = selected;
    return 0;
}
