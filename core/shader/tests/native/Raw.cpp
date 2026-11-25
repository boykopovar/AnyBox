#include <anybox/shader/Native.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "NativeFixtures.hpp"
#include <algorithm>

void CheckRawNativeFrontend() {
    using namespace Fixtures;
    using AnyBox::Shader::NativeFrontend;
    auto program = MakeNativeProgram();
    AnyBox::Graphics::Container::FssmView view(program);
    NativeFrontend wrapped(program);
    for (auto wave : {32u, 64u}) {
        NativeFrontend raw(view.GetCode(), wave);
        Require(raw.GetWaveSize() == wave);
        Require(std::ranges::equal(raw.GetCode(), wrapped.GetCode()));
        Require(raw.GetDecoded().instructions.size() == wrapped.GetDecoded().instructions.size());
        Require(raw.GetGraph().blocks.size() == wrapped.GetGraph().blocks.size());
        Require(raw.GetDecoded().code.data() == raw.GetCode().data());
        Require(raw.GetCode().data() != wrapped.GetCode().data());
    }
    for (auto wave : {0u, 16u, 31u, 33u, 63u, 65u, UINT32_MAX}) Reject<std::invalid_argument>([&] { NativeFrontend raw(view.GetCode(), wave); });
    for (std::size_t size = 0; size < 4; ++size) Reject<std::invalid_argument>([&] { NativeFrontend raw(view.GetCode().first(size), 32); });
    auto invalid = program;
    Set(invalid, 0x168, 0xBF820100);
    auto code = AnyBox::Graphics::Container::FssmView(invalid).GetCode();
    Reject<std::out_of_range>([&] { NativeFrontend raw(code, 32); });
}
