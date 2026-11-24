#ifndef ANYBOX_ACPHAL_MEMORY_TESTS_HPP
#define ANYBOX_ACPHAL_MEMORY_TESTS_HPP

#include <AcpHal.hpp>

#include <stdexcept>
#include <type_traits>

static_assert(std::is_same_v<decltype(&ApuAlloc), std::int32_t(*)(void**, APU_ADDRESS*, std::uint32_t, std::uint32_t, std::uint32_t)>);

namespace AnyBox::Tests {

inline constexpr std::uint32_t ApuCachedSize = 0x100000;
inline constexpr std::uint32_t ApuNonCachedSize = 0x40000;

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void ExpectException(TCallable&& callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected APU error was not raised");
}

void CheckAllocation();
void CheckConcurrency();
void CheckErrors();
void CheckMapping();

}

#endif
