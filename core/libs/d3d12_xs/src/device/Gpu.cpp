#include "Device.hpp"
#include <anybox/vulkan/Context.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <algorithm>
#include <bit>
#include <memory>

AnyBox::Graphics::Device::Device(ID3D12Device* host, const D3D12XboxCreateDeviceParameters& parameters) : vtable(&GetDeviceTable()), host(host), parameters(parameters) {
    auto nativeLuid = host->GetAdapterLuid();
    Vulkan::AdapterLuid luid{};
    static_assert(sizeof(nativeLuid) == sizeof(luid));
    std::memcpy(luid.data(), &nativeLuid, luid.size());
    auto created = std::make_unique<Vulkan::Context>(luid, PreparedQueueCount);
    constexpr auto usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto zero = std::make_unique<Vulkan::Buffer>(Vulkan::Buffer::Allocate(*created, 16, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    std::ranges::fill(zero->Map(), std::byte{});
    auto address = zero->GetAddress();
    if (address == 0 || address > 0xFFFFFFFFFFFFULL || address % 16 != 0) throw std::runtime_error("Vulkan null constant buffer address cannot be encoded in an Xbox descriptor");
    host->AddRef();
    gpuContext = created.release();
    nullConstantAddress = address;
    nullConstantBuffer = zero.release();
}

std::uint32_t AnyBox::Graphics::Device::ReservePreparedQueue() {
    constexpr auto mask = (std::uint32_t{1} << PreparedQueueCount) - 1;
    auto occupied = preparedQueues.load(std::memory_order_relaxed);
    for (;;) {
        auto available = ~occupied & mask;
        if (!available) throw std::runtime_error("No Vulkan queue is available for prepared compute");
        auto index = static_cast<std::uint32_t>(std::countr_zero(available));
        if (preparedQueues.compare_exchange_weak(occupied, occupied | (std::uint32_t{1} << index))) return index;
    }
}

void AnyBox::Graphics::Device::ReleasePreparedQueue(std::uint32_t index) noexcept {
    preparedQueues.fetch_and(~(std::uint32_t{1} << index), std::memory_order_release);
}
