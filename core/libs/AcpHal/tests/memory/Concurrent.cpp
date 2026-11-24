#include "Tests.hpp"

#include <array>
#include <barrier>
#include <cstddef>
#include <cstring>
#include <exception>
#include <thread>

void AnyBox::Tests::CheckConcurrency() {
    std::array<std::exception_ptr, 8> errors{};
    std::array<std::jthread, 8> threads;
    std::barrier start(static_cast<std::ptrdiff_t>(threads.size() + 1));
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::jthread([&errors, &start, i] {
            start.arrive_and_wait();
            try {
                for (std::uint32_t j = 0; j < 64; ++j) {
                    void* allocation = nullptr;
                    APU_ADDRESS physical = 0;
                    ApuAlloc(&allocation, &physical, 2048, 2048, j % 3);
                    Require(physical == ApuMapVirtualAddress(allocation), "Concurrent APU physical output changed");
                    Require(ApuIsVirtualAddressValid(allocation, 0), "Concurrent APU allocation is invalid");
                    auto value = static_cast<int>(i + 1);
                    std::memset(allocation, value, 2048);
                    auto* bytes = static_cast<const std::byte*>(allocation);
                    for (std::size_t k = 0; k < 2048; ++k) Require(bytes[k] == std::byte(value), "Concurrent APU allocations overlap");
                    Require(ApuMapApuAddress(physical) == allocation, "Concurrent APU translation failed");
                    ApuFree(allocation);
                }
            } catch (...) {
                errors[i] = std::current_exception();
            }
        });
    }
    start.arrive_and_wait();
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}
