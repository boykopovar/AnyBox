#include <xmem.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

std::byte storage;
unsigned calls = 0;
bool recurse = true;

void CheckRecursiveCall() {
    if (++calls != 1) throw std::runtime_error("The title hook was entered again");
}

void* Allocate(std::size_t size, std::uint64_t attributes) {
    if (!recurse) return &storage;
    CheckRecursiveCall();
    return XMemAlloc(size, attributes);
}

void Free(void* allocation, std::uint64_t attributes) {
    if (!recurse) return;
    CheckRecursiveCall();
    XMemFree(allocation, attributes);
}

template<class Call>
void ExpectRecursionError(Call call) {
    calls = 0;
    try {
        call();
    } catch (const std::invalid_argument& error) {
        if (std::string_view(error.what()) == "Title allocation hooks recurse into xmem" && calls == 1) return;
        throw;
    }
    throw std::runtime_error("Recursive call returned without an error");
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
        ExpectRecursionError([] { XMemAlloc(1, 0); });
        ExpectRecursionError([] { XMemFree(&storage, 0); });
        recurse = false;
        if (XMemAlloc(1, 0) != &storage) throw std::runtime_error("Allocation hook did not recover");
        XMemFree(&storage, 0);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
