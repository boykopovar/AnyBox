#pragma once

#include <anybox/vulkan/Buffer.hpp>
#include <anybox/PhysicalMemory.hpp>
#include <anybox/vulkan/MemoryTest.hpp>
#include <array>
#include <memory>

namespace AnyBox::Vulkan::Tests {
inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct LeaseDeleter {
    void operator()(IPhysicalPageLease* value) const noexcept { value->Release(); }
};
using Lease = std::unique_ptr<IPhysicalPageLease, LeaseDeleter>;

class Storage {
public:
    Storage();
    ~Storage();
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    void Map(std::span<const std::uint64_t> frames);
    std::span<std::byte> GetMemory() const noexcept;
    const std::array<std::uint64_t, 3>& GetFrames() const noexcept { return frames; }
    std::uint64_t GetExtraFrame() const noexcept { return extra; }

private:
    void release();
    std::array<std::uint64_t, 3> frames{};
    std::uint64_t extra = 0;
    bool allocated = false;
    bool extraAllocated = false;
    std::byte* address = nullptr;
};

void CheckLayout(Context& context, Storage& storage, std::span<const std::uint64_t> frames);
void CheckImages(Context& context);
void CheckImageViews(Context& context);
std::vector<std::uint32_t> CompileCompute(const char* source);
std::vector<std::uint32_t> CompileShader(const char* source, VkShaderStageFlagBits stage);
}
