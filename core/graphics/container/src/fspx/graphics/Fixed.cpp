#include "State.hpp"

AnyBox::Graphics::Container::SerializedGraphicsFixedState AnyBox::Graphics::Container::ReadSerializedGraphicsFixedState(std::span<const std::byte> state, std::span<const std::byte> auxiliary) {
    using namespace SerializedGraphics;
    auto registers = ReadRegisters(state);
    if (auxiliary.size() < 68 || auxiliary.size() % 4 != 0) throw std::invalid_argument("Xbox serialized graphics auxiliary payload is truncated or misaligned");
    if (ReadInteger(auxiliary, 4) != 4096) throw std::runtime_error("Xbox serialized graphics auxiliary format is not implemented");
    if ((Byte(auxiliary, 37) & 240) || (Byte(auxiliary, 38) & 192) || (Byte(auxiliary, 39) & 252) || (Byte(auxiliary, 40) & 240) || (Byte(auxiliary, 41) & 240) || (Byte(auxiliary, 42) & 240) || (Byte(auxiliary, 44) & 240)) throw std::runtime_error("Xbox serialized graphics auxiliary extensions are not implemented");
    auto context = registers.subspan(60);
    SerializedGraphicsFixedState result{};
    result.depth = ReadDepth(context);
    result.rasterizer = ReadRasterizer(context, auxiliary);
    result.targets = ReadBlend(context, auxiliary);
    auto coverage = ReadInteger(context, 112);
    result.alphaToCoverage = (coverage & 1) != 0 ? 2u - ((coverage >> 16) & 1) : 0u;
    result.independentBlend = (Byte(auxiliary, 44) & 1) != 0;
    result.sampleMask = ReadInteger(auxiliary, 28);
    result.targetCount = Byte(auxiliary, 37);
    if (result.targetCount > result.targets.size()) throw std::invalid_argument("Xbox serialized graphics target count exceeds its array");
    for (std::size_t index = 0; index < result.formats.size(); ++index) result.formats[index] = Byte(auxiliary, 16 + index);
    result.depthFormat = Byte(auxiliary, 36);
    result.samples = Byte(auxiliary, 40);
    if (result.samples == 0 || (result.samples & (result.samples - 1)) != 0) throw std::invalid_argument("Xbox serialized graphics sample count is invalid");
    result.sampleQuality = Byte(auxiliary, 41);
    result.topology = Byte(auxiliary, 38);
    result.stripCut = Byte(auxiliary, 39);
    result.flags = ReadInteger(auxiliary, 24);
    if (result.topology < 1 || result.topology > 3 || result.stripCut > 2 || (result.flags & ~1u) != 0) throw std::runtime_error("Xbox serialized graphics topology, strip cut or pipeline flags are not implemented");
    return result;
}
