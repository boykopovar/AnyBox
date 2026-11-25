#pragma once

#include "Image.hpp"
#include "Cpu.hpp"

namespace AnyBox::Relinker {
struct RelinkResult {
    std::uint64_t instructionCount = 0;
    std::vector<std::uint32_t> writebackAddresses;
    std::vector<std::uint32_t> monitorAddresses;
    std::vector<std::uint32_t> insertAddresses;
    std::vector<std::uint32_t> extractAddresses;
};
RelinkResult Relink(Image& image, const Cpu& cpu, bool forceCacheWriteback = false);
}
