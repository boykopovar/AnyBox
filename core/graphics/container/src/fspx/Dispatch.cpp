#include <anybox/graphics/container/FspxDispatch.hpp>
#include "../Integer.hpp"

AnyBox::Graphics::Container::SerializedComputeState AnyBox::Graphics::Container::ReadSerializedComputeState(std::span<const std::byte> data) {
    if (data.size() != 28) throw std::invalid_argument("Xbox serialized compute dispatch state size is invalid");
    if (ReadInteger(data, 0) != 0x4005CF00 || ReadInteger(data, 4) != 0x6C090102 || ReadInteger(data, 8) != 0) throw std::runtime_error("Xbox serialized compute dispatch state format is not implemented");
    auto xy = ReadInteger(data, 12);
    auto z = ReadInteger(data, 16);
    if ((z & ~0x10FFFFu) != 0) throw std::runtime_error("Xbox serialized compute dispatch flags are not implemented");
    auto resources = ReadInteger(data, 20);
    constexpr std::uint32_t supported = 0x40000000 | 0x800000 | 0x200000 | 0xFF000 | 0x3FF;
    if ((resources & ~supported) != 0) throw std::runtime_error("Xbox serialized compute register modes are not implemented");
    auto control = ReadInteger(data, 24);
    constexpr std::uint32_t supportedControl = 0x3E | 0x380 | 0x400 | 0x1800 | 0xFF8000;
    if ((control & ~supportedControl) != 0) throw std::runtime_error("Xbox serialized compute scratch, traps and exception modes are not implemented");
    if (((control >> 1) & 31) > 16 || ((control >> 11) & 3) == 3) throw std::invalid_argument("Xbox serialized compute initial register allocation is invalid");
    NativeComputeState shader{{xy & 65535, xy >> 16, z & 65535}, control, ((control >> 15) & 511) * 512, (resources >> 12) & 255, (resources & 0x200000) != 0, (resources & 0x800000) != 0};
    std::uint64_t threads = 1;
    for (auto dimension : shader.threads) {
        if (dimension == 0 || dimension > 1024) throw std::invalid_argument("Xbox serialized compute workgroup dimension is invalid");
        threads *= dimension;
    }
    if (threads > 1024) throw std::invalid_argument("Xbox serialized compute workgroup has too many threads");
    return {shader, (z & 0x100000) != 0 ? 32u : 64u};
}
