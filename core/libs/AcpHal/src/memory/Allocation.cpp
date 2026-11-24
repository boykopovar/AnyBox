#include "Heap.hpp"

#include <bit>
#include <iterator>
#include <new>
#include <stdexcept>

void* AnyBox::AcpHal::Heap::Allocate(std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) {
    if (size == 0) throw std::invalid_argument("APU allocation size must be nonzero");
    if (alignment < 4 || alignment > 0x10000 || !std::has_single_bit(alignment)) throw std::invalid_argument("Unsupported APU alignment");
    if (flags > 2) throw std::invalid_argument("Unsupported APU allocation flags");

    EnsureCreated();
    std::lock_guard lock(mutex);
    auto* base = state->memory;
    auto baseValue = reinterpret_cast<std::uintptr_t>(base);
    auto& blocks = flags == 2 ? state->nonCachedBlocks : state->cachedBlocks;
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->allocated) continue;
        auto padding = (alignment - (baseValue + it->offset) % alignment) % alignment;
        if (padding > it->size || size > it->size - padding) continue;
        auto start = it->offset + padding;
        auto remaining = it->size - padding - size;
        std::list<Block> prefix;
        std::list<Block> suffix;
        if (padding) prefix.push_back({it->offset, padding, false});
        if (remaining) suffix.push_back({start + size, remaining, false});
        blocks.splice(it, prefix);
        blocks.splice(std::next(it), suffix);
        *it = {start, size, true};
        return base + start;
    }
    throw std::bad_alloc();
}

void AnyBox::AcpHal::Heap::Free(void* allocation) {
    if (!allocation) return;
    EnsureCreated();
    std::lock_guard lock(mutex);
    auto address = reinterpret_cast<std::uintptr_t>(allocation);
    auto baseValue = reinterpret_cast<std::uintptr_t>(state->memory);
    if (address < baseValue || address - baseValue >= state->memorySize) throw std::invalid_argument("APU allocation is outside the heap");
    auto offset = address - baseValue;
    auto release = [offset](std::list<Block>& blocks) {
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            if (it->offset != offset || !it->allocated) continue;
            it->allocated = false;
            if (it != blocks.begin()) {
                auto previous = std::prev(it);
                if (!previous->allocated) {
                    previous->size += it->size;
                    blocks.erase(it);
                    it = previous;
                }
            }
            auto next = std::next(it);
            if (next != blocks.end() && !next->allocated) {
                it->size += next->size;
                blocks.erase(next);
            }
            return true;
        }
        return false;
    };
    if (release(state->cachedBlocks) || release(state->nonCachedBlocks)) return;
    throw std::invalid_argument("APU address is not the start of a live allocation");
}
