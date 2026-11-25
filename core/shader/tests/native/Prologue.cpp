#include <anybox/shader/Native.hpp>
#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include "NativeFixtures.hpp"
#include "../buffer/Target.hpp"
#include <Recompiler.hpp>
#include <array>
#include <bit>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

void CheckNativeHashPrologue() {
    using namespace Fixtures;
    using namespace AnyBox::Shader;
    const std::array<std::uint32_t, 10> words{0xD5801234, 0x03FE5678, 0x9ABCDEF0, 0xD5801357, 0x03FE2468, 0xABCD1234, 0xD5800000, 0x03FE0100, 0xAC0, 0xBF810000};
    auto bytes = std::bit_cast<std::array<std::byte, sizeof(words)>>(words);
    for (auto wave : {32u, 64u}) {
        NativeFrontend frontend(bytes, wave);
        auto instructions = frontend.GetDecoded().instructions;
        Require(instructions.size() == 4);
        for (std::size_t index = 0; index < 3; ++index) {
            const auto& instruction = instructions[index];
            Require(instruction.op == ShaderRecompiler::RdnaOpcode::VNop);
            Require(instruction.programCounter == index * 12 && instruction.wordCount == 3 && instruction.sourceCount == 0);
            Require(instruction.destination.kind == ShaderRecompiler::RdnaOperandKind::Null);
            for (std::size_t word = 0; word < 3; ++word) Require(instruction.rawWords[word] == words[index * 3 + word]);
        }
        Require(instructions[3].programCounter == 36 && instructions[3].op == ShaderRecompiler::RdnaOpcode::SEndpgm);
        AnyBox::Graphics::Container::NativeComputeState state{{1, 1, 1}, 0, 0, 0xC0, true, false};
        auto compiled = CompileNativeCompute(frontend, state, BufferTests::Target(), {0, 0, 0, 128});
        CheckSpirvValidation(compiled.code);
    }
    for (std::size_t size = 0; size < 36; size += 4) Reject<std::exception>([&] { NativeFrontend rejected(std::span(bytes).first(size), 32); });
    for (auto selection : {1u, 2u, 3u}) {
        auto variant = bytes;
        Set(variant, 4, words[1] | ((selection & 1) << 16));
        Set(variant, 16, words[4] | ((selection & 2) << 15));
        NativeFrontend frontend(variant, 32);
        Require(frontend.GetDecoded().instructions.size() == 4);
        Require(frontend.GetDecoded().instructions[0].rawWords[1] == (words[1] | ((selection & 1) << 16)));
        Require(frontend.GetDecoded().instructions[1].rawWords[1] == (words[4] | ((selection & 2) << 15)));
    }
    for (auto offset : {0u, 4u, 12u, 16u, 24u, 28u}) {
        auto invalid = bytes;
        Set(invalid, offset, 0);
        Reject<std::exception>([&] { NativeFrontend rejected(invalid, 32); });
    }
    auto invalidTarget = bytes;
    Set(invalidTarget, 36, 0xBF82FFF7);
    Reject<std::exception>([&] { NativeFrontend rejected(invalidTarget, 32); });
}
