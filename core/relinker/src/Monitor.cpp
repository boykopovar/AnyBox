#include <anybox/relinker/Decoder.hpp>
#include <algorithm>
#include <stdexcept>

std::optional<AnyBox::Relinker::EncodedInstruction> AnyBox::Relinker::Decoder::LowerMonitor(const DecodedInstruction& instruction) const {
    const auto& original = instruction.native;
    auto wait = original.mnemonic == ZYDIS_MNEMONIC_MWAITX;
    if (!wait && original.mnemonic != ZYDIS_MNEMONIC_MONITORX) return std::nullopt;
    if (original.length < 3 || original.length > ZYDIS_MAX_INSTRUCTION_LENGTH) throw std::invalid_argument("MONITORX or MWAITX instruction encoding is inconsistent");
    EncodedInstruction replacement;
    replacement.length = original.length;
    std::fill_n(replacement.bytes.begin(), replacement.length, std::byte{0x90});
    if (wait) replacement.bytes[0] = std::byte{0xF3};

    auto bytes = std::span(replacement.bytes).first(replacement.length);
    for (std::size_t position = 0; position < bytes.size();) {
        auto verified = Decode(bytes.subspan(position));
        auto expected = wait && position == 0 ? ZYDIS_MNEMONIC_PAUSE : ZYDIS_MNEMONIC_NOP;
        if (verified.native.mnemonic != expected) throw std::runtime_error("MONITORX or MWAITX lowering produced an unexpected instruction");
        position += verified.native.length;
    }
    return replacement;
}
