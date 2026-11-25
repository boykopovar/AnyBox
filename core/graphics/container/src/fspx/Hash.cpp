#include "Hash.hpp"
#include "../Integer.hpp"

std::array<std::byte, 16> AnyBox::Graphics::Container::ReadSerializedCodeHash(std::span<const std::byte> code) {
    if (code.size() < 36) throw std::invalid_argument("Xbox serialized shader hash prologue is truncated");
    std::array<std::byte, 16> result{};
    for (std::size_t index = 0; index < 2; ++index) {
        auto first = ReadInteger(code, index * 12);
        auto second = ReadInteger(code, index * 12 + 4);
        if ((first & 0xFFFF0000u) != 0xD5800000u || (second & 0xFFFE0100u) != 0x03FE0100u) throw std::runtime_error("Xbox serialized shader hash prologue encoding is not implemented");
        auto high = (second & 255) | ((second >> 1) & 0xFF00);
        WriteInteger(result, index * 8, (first & 65535) | (high << 16));
        WriteInteger(result, index * 8 + 4, ReadInteger(code, index * 12 + 8));
    }
    if (ReadInteger(code, 24) != 0xD5800000 || ReadInteger(code, 28) != 0x03FE0100) throw std::runtime_error("Xbox serialized shader hash prologue terminator is not implemented");
    return result;
}
