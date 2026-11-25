#include "State.hpp"
#include <algorithm>

namespace {
std::uint32_t StencilOperation(std::uint32_t value) {
    constexpr std::array<std::uint32_t, 16> encodings{0, 1, 3, 5, 6, 7, 8, 9, 2, 4, 10, 11, 12, 13, 14, 15};
    auto found = std::ranges::find(encodings, value);
    if (found == encodings.end()) throw std::runtime_error("Xbox serialized extended stencil operation is not implemented");
    return static_cast<std::uint32_t>(found - encodings.begin()) + 1;
}
}

AnyBox::Graphics::Container::SerializedDepthState AnyBox::Graphics::Container::SerializedGraphics::ReadDepth(std::span<const std::byte> context) {
    auto control = ReadInteger(context, 96);
    auto operations = ReadInteger(context, 100);
    if ((control & 0xC0000000) != 0) throw std::runtime_error("Xbox serialized extended depth and stencil state is not implemented");
    SerializedDepthState result{};
    result.depthEnable = (control & 2) != 0;
    result.depthWrite = (control & 4) != 0;
    result.comparison = ((control >> 4) & 7) + 1;
    result.stencilEnable = (control & 1) != 0;
    result.boundsEnable = (control & 8) != 0;
    result.separateBackFace = (control & 128) != 0;
    result.readMask = Byte(context, 105);
    result.writeMask = Byte(context, 106);
    result.backReadMask = Byte(context, 109);
    result.backWriteMask = Byte(context, 110);
    result.reference = Byte(context, 104);
    result.backReference = Byte(context, 108);
    result.operationValue = Byte(context, 107);
    result.backOperationValue = Byte(context, 111);
    result.front = {StencilOperation(operations & 15), StencilOperation((operations >> 8) & 15), StencilOperation((operations >> 4) & 15), ((control >> 8) & 7) + 1};
    result.back = {StencilOperation((operations >> 12) & 15), StencilOperation((operations >> 20) & 15), StencilOperation((operations >> 16) & 15), ((control >> 20) & 7) + 1};
    return result;
}
