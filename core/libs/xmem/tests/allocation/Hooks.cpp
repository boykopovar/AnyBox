#include <xmem.hpp>

#include <array>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

thread_local std::array<std::byte, 64> storage{};
thread_local std::uint32_t allocationCalls = 0;
thread_local std::uint32_t freeCalls = 0;

void* Allocate(std::size_t size, std::uint64_t attributes) {
    ++allocationCalls;
    if (size == 0 && attributes == 0) return nullptr;
    if (size != storage.size() || attributes != 0x0123456789ABCDEF) throw std::runtime_error("Allocation arguments changed");
    return storage.data();
}

void Free(void* allocation, std::uint64_t attributes) {
    ++freeCalls;
    if (allocation == nullptr && attributes == 0) return;
    if (allocation != storage.data() || attributes != 0xFEDCBA9876543210) throw std::runtime_error("Free arguments changed");
}

void Check() {
    auto* allocation = XMemAlloc(storage.size(), 0x0123456789ABCDEF);
    if (allocation != storage.data()) throw std::runtime_error("Title allocation result changed");
    XMemFree(allocation, 0xFEDCBA9876543210);
    if (allocationCalls != 1 || freeCalls != 1) throw std::runtime_error("Title hooks were not called exactly once");
    if (XMemAlloc(0, 0) != nullptr) throw std::runtime_error("Title allocation failure was not preserved");
    XMemFree(nullptr, 0);
    if (allocationCalls != 2 || freeCalls != 2) throw std::runtime_error("Null operations bypassed title hooks");
    bool caught = false;
    try {
        XMemAlloc(1, 0);
    } catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "Allocation arguments changed";
    }
    if (!caught || allocationCalls != 3) throw std::runtime_error("Title exception was not propagated");
}

}

struct TitleHooks {
    decltype(&Allocate) Allocation;
    decltype(&Free) Release;
};

extern "C" {
TitleHooks __XMemAllocationHooks__{Allocate, Free};
}

int main() {
    try {
        std::array<std::exception_ptr, 8> errors{};
        std::array<std::jthread, 8> threads;
        std::barrier start(static_cast<std::ptrdiff_t>(threads.size() + 1));
        for (std::size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::jthread([&errors, &start, i] {
                start.arrive_and_wait();
                try {
                    Check();
                } catch (...) {
                    errors[i] = std::current_exception();
                }
            });
        }
        start.arrive_and_wait();
        Check();
        for (auto& thread : threads) thread.join();
        for (const auto& error : errors) if (error) std::rethrow_exception(error);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
