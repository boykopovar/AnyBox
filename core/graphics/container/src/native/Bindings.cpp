#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/graphics/container/NativeCompute.hpp>
#include <anybox/graphics/container/NativeStage.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "../Integer.hpp"

namespace {
using namespace AnyBox::Graphics::Container;

NativeRootKind RootKind(std::uint32_t value) {
    switch (value) {
    case 0: case 1: case 3: case 4: case 5: case 6: return static_cast<NativeRootKind>(value);
    default: throw std::runtime_error("Xbox native root binding kind is not implemented");
    }
}

std::span<const std::byte> Table(std::span<const std::byte> program, const FssmView& view, std::size_t pointer, std::size_t countOffset) {
    auto count = ReadInteger(program, countOffset);
    if (count == 0) return {};
    auto offset = ReadInteger(program, pointer) | (static_cast<std::uint64_t>(ReadInteger(program, pointer + 4)) << 32);
    auto size = static_cast<std::uint64_t>(count) * 20;
    auto codeStart = view.GetCodeSectionOffset() + 16;
    auto codeEnd = codeStart + view.GetCodeSectionSize();
    if (offset > UINT64_MAX - 16) throw std::invalid_argument("Xbox native binding table offset overflows");
    offset += 16;
    if (offset % 4 != 0 || offset < 0x148 || offset > program.size() || size > program.size() - offset) throw std::invalid_argument("Xbox native binding table range is invalid");
    if (offset < codeEnd && codeStart < offset + size) throw std::invalid_argument("Xbox native binding table overlaps its instruction stream");
    return program.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size));
}

std::uint8_t Byte(std::span<const std::byte> entry, std::size_t index) {
    return std::to_integer<std::uint8_t>(entry[index]);
}
}

AnyBox::Graphics::Container::NativeBindings AnyBox::Graphics::Container::ReadNativeComputeBindings(std::span<const std::byte> program) {
    (void)ReadNativeComputeState(program);
    return ReadNativeBindings(program);
}

AnyBox::Graphics::Container::NativeBindings AnyBox::Graphics::Container::ReadNativeBindings(std::span<const std::byte> program) {
    (void)ReadNativeStageState(program);
    FssmView view(program);
    auto roots = Table(program, view, 0xE8, 0xF8);
    auto resources = Table(program, view, 0xF0, 0xFC);
    if (!roots.empty() && !resources.empty()) {
        auto rootStart = static_cast<std::size_t>(roots.data() - program.data());
        auto resourceStart = static_cast<std::size_t>(resources.data() - program.data());
        if (rootStart < resourceStart + resources.size() && resourceStart < rootStart + roots.size()) throw std::invalid_argument("Xbox native binding tables overlap");
    }
    NativeBindings result;
    result.roots.reserve(roots.size() / 20);
    result.resources.reserve(resources.size() / 20);
    for (std::size_t offset = 0; offset < roots.size(); offset += 20) {
        auto entry = roots.subspan(offset, 20);
        if (Byte(entry, 18) != 0 || Byte(entry, 19) != 0) throw std::runtime_error("Xbox native root binding flags are not implemented");
        result.roots.push_back({RootKind(ReadInteger(entry, 0)), ReadInteger(entry, 4), ReadInteger(entry, 8), ReadInteger(entry, 12), Byte(entry, 16), Byte(entry, 17)});
    }
    for (std::size_t offset = 0; offset < resources.size(); offset += 20) {
        auto entry = resources.subspan(offset, 20);
        result.resources.push_back(ReadNativeResourceBinding(entry));
    }
    return result;
}
