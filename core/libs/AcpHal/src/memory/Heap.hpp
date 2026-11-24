#ifndef ANYBOX_ACPHAL_MEMORY_HEAP_HPP
#define ANYBOX_ACPHAL_MEMORY_HEAP_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

namespace AnyBox::AcpHal {

class Heap {
public:
    void Create(std::uint32_t cachedSize, std::uint32_t nonCachedSize);
    void* Allocate(std::uint32_t size, std::uint32_t alignment, std::uint32_t flags);
    void Free(void* allocation);
    bool IsValid(const void* address, std::uint32_t physicalAlignment);
    std::uint32_t ToApu(const void* address);
    void* ToVirtual(std::uint32_t address);

private:
    struct Block {
        std::size_t offset;
        std::size_t size;
        bool allocated;
    };

    struct State {
        std::byte* memory = nullptr;
        std::uint32_t memorySize = 0;
        std::uint32_t busBase = 0;
        std::list<Block> cachedBlocks;
        std::list<Block> nonCachedBlocks;
    };

    void CreateInternal(std::uint32_t cachedSize, std::uint32_t nonCachedSize, bool allowExisting);
    void EnsureCreated();
    static std::shared_ptr<State> CreateState(std::uint32_t cachedSize, std::uint32_t nonCachedSize);
    static const Block* FindAllocated(const State& state, std::size_t offset);

    std::mutex mutex;
    std::condition_variable condition;
    std::shared_ptr<State> state;
    bool creating = false;
    std::thread::id creatingThread;
};

Heap& GetHeap();

}

#endif
