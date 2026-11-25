#include "Tests.hpp"
#include "Fixtures.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace {
struct Release {
    void operator()(void* value) const noexcept { if (!VirtualFree(value, 0, MEM_RELEASE)) std::terminate(); }
};

void Execute(std::span<const std::byte> code) {
    std::unique_ptr<void, Release> memory(VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!memory) AnyBox::Windows::ThrowLastError("Allocate lowered monitor test code");
    std::memcpy(memory.get(), code.data(), code.size());
    DWORD old = 0;
    if (!VirtualProtect(memory.get(), 4096, PAGE_EXECUTE_READ, &old)) AnyBox::Windows::ThrowLastError("Protect lowered monitor test code");
    if (!FlushInstructionCache(GetCurrentProcess(), memory.get(), code.size())) AnyBox::Windows::ThrowLastError("Flush lowered monitor test code");
    using Function = void (*)();
    Function function;
    auto* address = memory.get();
    static_assert(sizeof(function) == sizeof(address));
    std::memcpy(&function, &address, sizeof(function));
    function();
}
}

void AnyBox::Relinker::Tests::CheckMonitor() {
    Decoder decoder;
    const std::vector<std::vector<std::uint8_t>> cases{
        {0x0F, 0x01, 0xFA},
        {0x0F, 0x01, 0xFB},
        {0x67, 0x0F, 0x01, 0xFA}
    };
    for (const auto& bytes : cases) {
        auto code = std::as_bytes(std::span(bytes));
        auto original = decoder.Decode(code);
        auto wait = original.native.mnemonic == ZYDIS_MNEMONIC_MWAITX;
        Require(wait || original.native.mnemonic == ZYDIS_MNEMONIC_MONITORX, "Monitor fixture is not MONITORX or MWAITX");
        auto replacement = decoder.LowerMonitor(original);
        Require(replacement && replacement->length == bytes.size(), "Monitor lowering changed the instruction length");
        auto lowered = std::span(replacement->bytes).first(replacement->length);
        auto first = decoder.Decode(lowered);
        Require(first.native.mnemonic == (wait ? ZYDIS_MNEMONIC_PAUSE : ZYDIS_MNEMONIC_NOP), "Monitor lowering did not start with the expected instruction");
        for (auto position = static_cast<std::size_t>(first.native.length); position < lowered.size();) {
            auto padding = decoder.Decode(lowered.subspan(position));
            Require(padding.native.mnemonic == ZYDIS_MNEMONIC_NOP, "Monitor lowering padded with a non-NOP instruction");
            position += padding.native.length;
        }
    }
    const std::array<std::uint8_t, 4> writeback{0x66, 0x0F, 0xAE, 0x30};
    Require(!decoder.LowerMonitor(decoder.Decode(std::as_bytes(std::span(writeback)))), "CLWB was mistaken for MONITORX or MWAITX");

    const std::array<std::uint8_t, 7> code{0x0F, 0x01, 0xFA, 0x0F, 0x01, 0xFB, 0xC3};
    auto original = Fixture(code);
    Image image(original);
    auto result = Relink(image, Cpu(CpuState{}));
    Require(result.monitorAddresses == std::vector<std::uint32_t>{0x1000, 0x1003}, "PE pass did not lower MONITORX and MWAITX at their boundaries");
    const std::array<std::uint8_t, 7> expected{0x90, 0x90, 0x90, 0xF3, 0x90, 0x90, 0xC3};
    Require(std::ranges::equal(image.GetBytes().subspan(512, expected.size()), std::as_bytes(std::span(expected))), "PE monitor replacement encoding changed");
    Require(std::ranges::equal(image.GetBytes().subspan(1024), std::span(original).subspan(1024)), "PE monitor lowering changed its unwind data");
    auto lowered = std::vector<std::byte>(image.GetBytes().begin(), image.GetBytes().end());
    Require(Relink(image, Cpu(CpuState{})).monitorAddresses.empty() && std::ranges::equal(image.GetBytes(), lowered), "PE monitor lowering is not idempotent");

    CpuState nativeState;
    nativeState.extended[2] = 1U << 29;
    Image nativeImage(original);
    Require(Relink(nativeImage, Cpu(nativeState)).monitorAddresses.empty() && std::ranges::equal(nativeImage.GetBytes(), original), "Supported MONITORX and MWAITX were rewritten");

    Execute(image.GetBytes().subspan(512, expected.size()));
}
