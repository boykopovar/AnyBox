#pragma once

#include "Functions.hpp"
#include <anybox/platform/Windows.hpp>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <mutex>
#include <variant>

namespace AnyBox::Vulkan {
using AdapterLuid = std::array<std::byte, VK_LUID_SIZE>;

class AdapterNotFound : public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

class AdapterIncompatible : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Context {
public:
    explicit Context(std::uint32_t adapterIndex, std::uint32_t queueCount = 1);
    explicit Context(const AdapterLuid& adapterLuid, std::uint32_t queueCount = 1);
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    VkDevice GetDevice() const noexcept { return device; }
    VkPhysicalDevice GetPhysicalDevice() const noexcept { return physicalDevice; }
    const DeviceFunctions& GetFunctions() const noexcept { return deviceFunctions; }
    const VkPhysicalDeviceProperties& GetProperties() const noexcept { return properties; }
    const VkPhysicalDeviceFeatures& GetEnabledFeatures() const noexcept { return enabledFeatures; }
    const VkPhysicalDeviceSubgroupProperties& GetSubgroupProperties() const noexcept { return subgroups; }
    const VkPhysicalDeviceFloatControlsProperties& GetFloatControls() const noexcept { return floatControls; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const noexcept { return memoryProperties; }
    VkDeviceSize GetHostPointerAlignment() const noexcept { return hostPointerAlignment; }
    std::uint32_t GetQueueFamily() const noexcept { return queueFamily; }
    VkQueue GetQueue(std::uint32_t index) const;
    std::uint32_t GetQueueCount() const noexcept { return static_cast<std::uint32_t>(queues.size()); }
    VkExternalBufferProperties GetExternalBufferProperties(VkBufferUsageFlags usage) const;
    VkImageFormatProperties GetImageFormatProperties(const VkPhysicalDeviceImageFormatInfo2& description) const;
    bool SupportsCubeArrays() const noexcept { return cubeArrays; }
    bool SupportsByteStorage() const noexcept { return byteStorage; }
    VkExternalSemaphoreProperties GetD3D12FenceProperties() const;
    const AdapterLuid& GetAdapterLuid() const noexcept { return adapterLuid; }
    void Submit(std::uint32_t queueIndex, const VkSubmitInfo& submission, VkFence fence = VK_NULL_HANDLE);

private:
    using AdapterSelection = std::variant<std::uint32_t, AdapterLuid>;
    Context(const AdapterSelection& selection, std::uint32_t queueCount);
    void loadInstanceFunctions();
    void loadDeviceFunctions();
    void findAdapter(const AdapterSelection& selection);
    void selectAdapter(const AdapterSelection& selection, std::uint32_t queueCount);
    void createDevice(std::uint32_t queueCount);
    void release() noexcept;
    HMODULE module = nullptr;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    InstanceFunctions instanceFunctions{};
    DeviceFunctions deviceFunctions{};
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures enabledFeatures{};
    VkPhysicalDeviceSubgroupProperties subgroups{};
    VkPhysicalDeviceFloatControlsProperties floatControls{};
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    VkDeviceSize hostPointerAlignment = 0;
    std::uint32_t queueFamily = 0;
    std::vector<VkQueue> queues;
    std::unique_ptr<std::mutex[]> queueMutexes;
    AdapterLuid adapterLuid{};
    bool cubeArrays = false;
    bool byteStorage = false;
};
}
