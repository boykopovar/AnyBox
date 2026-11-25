#include "State.hpp"

std::span<const std::byte> AnyBox::Graphics::Container::SerializedGraphics::ReadRegisters(std::span<const std::byte> state) {
    if (state.size() < 44 || state.size() % 4 != 0) throw std::invalid_argument("Xbox serialized graphics state payload is truncated or misaligned");
    auto version = ReadInteger(state, 8) >> 4;
    if (version != 0x10004 && version != 0x10005 && version != 0x10006) throw std::runtime_error("Xbox serialized graphics state version is not implemented");
    if ((ReadInteger(state, 0) & 0xFFFFFF80u) != 0x4005D800) throw std::invalid_argument("Xbox serialized graphics packet header is invalid");
    if ((ReadInteger(state, 16) & 0xFFFFFFF0u) != 0xB50516A0) throw std::invalid_argument("Xbox serialized graphics state signature is invalid");
    if ((ReadInteger(state, 4) & 3) != 0) throw std::runtime_error("Xbox serialized tessellation and geometry state is not implemented");
    auto lengths = ReadInteger(state, 40);
    auto registers = lengths & 65535;
    auto hashes = lengths >> 16;
    if (registers < 216 || registers % 4 != 0 || hashes % 4 != 0 || static_cast<std::uint64_t>(28) + registers + hashes != state.size()) throw std::invalid_argument("Xbox serialized graphics register and hash ranges are invalid");
    return state.subspan(28, registers);
}

std::span<const std::byte> AnyBox::Graphics::Container::SerializedGraphics::ReadHashes(std::span<const std::byte> state) {
    auto registers = ReadRegisters(state);
    return state.subspan(28 + registers.size());
}
