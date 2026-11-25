#pragma once

#include <Zydis/Zydis.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace AnyBox::Relinker {
struct DecodedInstruction {
    ZydisDecodedInstruction native{};
    std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
};
struct EncodedInstruction {
    std::array<std::byte, ZYDIS_MAX_INSTRUCTION_LENGTH> bytes{};
    std::size_t length = 0;
};
class Decoder {
public:
    Decoder();
    DecodedInstruction Decode(std::span<const std::byte> code) const;
    std::optional<EncodedInstruction> LowerWriteback(const DecodedInstruction& instruction, std::span<const std::byte> code) const;
    std::optional<EncodedInstruction> LowerMonitor(const DecodedInstruction& instruction) const;

private:
    ZydisDecoder decoder{};
};
}
