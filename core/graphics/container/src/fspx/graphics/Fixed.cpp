#include "State.hpp"

AnyBox::Graphics::Container::SerializedGraphicsFixedState AnyBox::Graphics::Container::ReadSerializedGraphicsFixedState(std::span<const std::byte> state, std::span<const std::byte> auxiliary) {
    using namespace SerializedGraphics;
    if (state.size() < 44 || state.size() % 4 != 0 || auxiliary.size() < 68 || auxiliary.size() % 4 != 0) throw std::invalid_argument("Xbox serialized graphics state payload is truncated or misaligned");
    auto version = ReadInteger(state, 8) >> 4;
    if (version != 0x10004 && version != 0x10005 && version != 0x10006) throw std::runtime_error("Xbox serialized graphics state version is not implemented");
    if ((ReadInteger(state, 0) & 0xFFFFFF80u) != 0x4005D800) throw std::invalid_argument("Xbox serialized graphics packet header is invalid");
    if ((ReadInteger(state, 16) & 0xFFFFFFF0u) != 0xB50516A0) throw std::invalid_argument("Xbox serialized graphics state signature is invalid");
    if ((ReadInteger(state, 4) & 3) != 0) throw std::runtime_error("Xbox serialized tessellation and geometry state is not implemented");
    auto lengths = ReadInteger(state, 40);
    auto registers = lengths & 65535;
    auto hashes = lengths >> 16;
    if (registers < 216 || registers % 4 != 0 || hashes % 4 != 0 || static_cast<std::uint64_t>(28) + registers + hashes != state.size()) throw std::invalid_argument("Xbox serialized graphics register and hash ranges are invalid");
    if (ReadInteger(auxiliary, 4) != 4096) throw std::runtime_error("Xbox serialized graphics auxiliary format is not implemented");
    if ((Byte(auxiliary, 37) & 240) || (Byte(auxiliary, 38) & 192) || (Byte(auxiliary, 39) & 252) || (Byte(auxiliary, 40) & 240) || (Byte(auxiliary, 41) & 240) || (Byte(auxiliary, 42) & 240) || (Byte(auxiliary, 44) & 240)) throw std::runtime_error("Xbox serialized graphics auxiliary extensions are not implemented");
    auto context = state.subspan(88, registers - 60);
    SerializedGraphicsFixedState result{};
    result.depth = ReadDepth(context);
    result.rasterizer = ReadRasterizer(context, auxiliary);
    result.targets = ReadBlend(context, auxiliary);
    result.alphaToCoverage = (Byte(auxiliary, 44) & 1) != 0;
    result.independentBlend = true;
    const auto& first = result.targets.front();
    for (auto factor : {first.source, first.destination, first.sourceAlpha, first.destinationAlpha}) if (factor >= 16 && factor <= 19) result.independentBlend = false;
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
