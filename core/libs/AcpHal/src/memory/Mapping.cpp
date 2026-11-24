#include "Heap.hpp"

const AnyBox::AcpHal::Heap::Block* AnyBox::AcpHal::Heap::FindAllocated(const State& state, std::size_t offset) {
    auto find = [offset](const std::list<Block>& blocks) -> const Block* {
        for (const auto& block : blocks) {
            if (block.allocated && offset >= block.offset && offset - block.offset < block.size) return &block;
        }
        return nullptr;
    };
    if (auto* block = find(state.cachedBlocks)) return block;
    return find(state.nonCachedBlocks);
}

bool AnyBox::AcpHal::Heap::IsValid(const void* address, std::uint32_t physicalAlignment) {
    if (!address) return false;
    EnsureCreated();
    std::lock_guard lock(mutex);
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto baseValue = reinterpret_cast<std::uintptr_t>(state->memory);
    if (value < baseValue || value - baseValue >= state->memorySize) return false;
    auto offset = value - baseValue;
    if (!FindAllocated(*state, offset)) return false;
    auto apuAddress = state->busBase + static_cast<std::uint32_t>(offset);
    return physicalAlignment == 0 || apuAddress % physicalAlignment == 0;
}

std::uint32_t AnyBox::AcpHal::Heap::ToApu(const void* address) {
    if (!address) return 0;
    EnsureCreated();
    std::lock_guard lock(mutex);
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto baseValue = reinterpret_cast<std::uintptr_t>(state->memory);
    if (value < baseValue || value - baseValue >= state->memorySize) return 0;
    auto offset = value - baseValue;
    if (!FindAllocated(*state, offset)) return 0;
    return state->busBase + static_cast<std::uint32_t>(offset);
}

void* AnyBox::AcpHal::Heap::ToVirtual(std::uint32_t address) {
    if (address == 0) return nullptr;
    EnsureCreated();
    std::lock_guard lock(mutex);
    if (address < state->busBase || address - state->busBase >= state->memorySize) return nullptr;
    auto offset = address - state->busBase;
    if (!FindAllocated(*state, offset)) return nullptr;
    return state->memory + offset;
}
