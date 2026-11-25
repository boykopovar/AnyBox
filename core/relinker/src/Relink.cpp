#include <anybox/relinker/Relink.hpp>
#include <anybox/relinker/Decoder.hpp>
#include <algorithm>
#include <string>
#include "ControlFlow.hpp"

AnyBox::Relinker::RelinkResult AnyBox::Relinker::Relink(Image& image, const Cpu& cpu, bool forceCacheWriteback) {
    Decoder decoder;
    RelinkResult result;
    auto entries = image.EntryPoints();
    auto imports = image.ImportSlots();
    std::vector<LoweringSite> lowerings;
    std::uint64_t unresolvedCount = 0;
    std::uint32_t firstUnresolved = 0;
    std::vector<std::uint8_t> visited(image.GetBytes().size());
    while (!entries.empty()) {
        auto address = entries.back();
        entries.pop_back();
        while (image.IsExecutable(address)) {
            auto offset = image.FileOffset(address, 1);
            if (visited[offset] != 0) {
                if (visited[offset] == 2) throw std::runtime_error("PE control flow enters an instruction interior at RVA " + std::to_string(address));
                break;
            }
            auto code = image.Code(address);
            DecodedInstruction instruction;
            try { instruction = decoder.Decode(code); }
            catch (const std::exception& error) { throw std::runtime_error("PE instruction RVA " + std::to_string(address) + ": " + error.what()); }
            auto length = instruction.native.length;
            for (std::size_t index = 0; index < length; ++index) {
                if (visited[offset + index] != 0) throw std::runtime_error("PE decoded instruction ranges overlap");
                visited[offset + index] = index == 0 ? 1 : 2;
            }
            ++result.instructionCount;
            auto category = instruction.native.meta.category;
            if (category == ZYDIS_CATEGORY_CALL || category == ZYDIS_CATEGORY_COND_BR || category == ZYDIS_CATEGORY_UNCOND_BR) {
                if (!FollowTransfer(image, instruction, address, imports, entries)) {
                    if (unresolvedCount == 0) firstUnresolved = address;
                    ++unresolvedCount;
                }
            }
            if (!cpu.Supports(instruction) || (forceCacheWriteback && instruction.native.mnemonic == ZYDIS_MNEMONIC_CLWB)) {
                auto replacement = decoder.LowerWriteback(instruction, code);
                if (replacement) {
                    auto bytes = std::span(replacement->bytes).first(replacement->length);
                    cpu.Require(decoder.Decode(bytes), address);
                    std::ranges::copy(bytes, image.GetBytes().begin() + static_cast<std::ptrdiff_t>(offset));
                    result.writebackAddresses.push_back(address);
                } else if (auto monitor = decoder.LowerMonitor(instruction)) {
                    auto bytes = std::span(monitor->bytes).first(monitor->length);
                    for (std::size_t position = 0; position < bytes.size();) {
                        auto emitted = decoder.Decode(bytes.subspan(position));
                        cpu.Require(emitted, address);
                        position += emitted.native.length;
                    }
                    std::ranges::copy(bytes, image.GetBytes().begin() + static_cast<std::ptrdiff_t>(offset));
                    result.monitorAddresses.push_back(address);
                } else if (instruction.native.mnemonic == ZYDIS_MNEMONIC_INSERTQ) {
                    auto allocation = image.GetStackAllocation(address);
                    if (allocation % 8 != 0) throw std::runtime_error("PE lowering call site has an unsupported stack alignment");
                    auto function = LowerImmediateInsert(instruction, static_cast<std::uint8_t>(32 + (allocation & 8)));
                    for (std::size_t position = 0; position < function.code.size();) {
                        auto emitted = decoder.Decode(std::span(function.code).subspan(position));
                        cpu.Require(emitted, address);
                        position += emitted.native.length;
                    }
                    lowerings.push_back({address, length, std::move(function)});
                    result.insertAddresses.push_back(address);
                } else if (instruction.native.mnemonic == ZYDIS_MNEMONIC_EXTRQ) {
                    auto function = LowerImmediateExtract(instruction);
                    for (std::size_t position = 0; position < function.code.size();) {
                        auto emitted = decoder.Decode(std::span(function.code).subspan(position));
                        cpu.Require(emitted, address);
                        position += emitted.native.length;
                    }
                    lowerings.push_back({address, length, std::move(function)});
                    result.extractAddresses.push_back(address);
                } else cpu.Require(instruction, address);
            }
            if (category == ZYDIS_CATEGORY_UNCOND_BR || category == ZYDIS_CATEGORY_RET || instruction.native.mnemonic == ZYDIS_MNEMONIC_INT3 || instruction.native.mnemonic == ZYDIS_MNEMONIC_UD2) break;
            if (address > UINT32_MAX - length) throw std::runtime_error("PE instruction address overflows");
            address += length;
        }
    }
    if (unresolvedCount != 0) throw std::runtime_error("Unresolved PE control flow transfers: " + std::to_string(unresolvedCount) + "; first RVA " + std::to_string(firstUnresolved));
    ZydisDecoder scanner{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&scanner, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64))) throw std::runtime_error("Cannot initialize the PE cache instruction scan");
    for (const auto& section : image.GetSections()) {
        if ((section.flags & 0x20000000) == 0 || section.size == 0) continue;
        auto code = image.Code(section.address);
        for (std::size_t index = 0; index < code.size(); ++index) {
            if (visited[section.offset + index] != 0 || code[index] != std::byte{0x66}) continue;
            ZydisDecodedInstruction candidate{};
            if (ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&scanner, nullptr, code.data() + index, code.size() - index, &candidate)) && candidate.mnemonic == ZYDIS_MNEMONIC_CLWB) throw std::runtime_error("Unclassified CLWB at PE RVA " + std::to_string(section.address + index));
        }
    }
    image.AppendLowerings(lowerings);
    if (!result.writebackAddresses.empty() || !result.monitorAddresses.empty()) image.Finalize();
    return result;
}
