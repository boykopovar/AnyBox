#include "TitleHooks.hpp"

#include <xmem.hpp>

#include <stdexcept>

namespace {

class HookCall {
public:
    explicit HookCall(bool& active) : active(active) {
        if (active) throw std::invalid_argument("Title allocation hooks recurse into xmem");
        active = true;
    }

    ~HookCall() { active = false; }

    HookCall(const HookCall&) = delete;
    HookCall& operator=(const HookCall&) = delete;

private:
    bool& active;
};

}

void* XMemAlloc(std::size_t size, std::uint64_t attributes) noexcept(false) {
    const auto& hooks = AnyBox::XMem::GetTitleHooks();
    if (hooks.Allocate) {
        static thread_local bool active = false;
        const HookCall call(active);
        return hooks.Allocate(size, attributes);
    }
    return XMemAllocDefault(size, attributes);
}

void XMemFree(void* allocation, std::uint64_t attributes) noexcept(false) {
    const auto& hooks = AnyBox::XMem::GetTitleHooks();
    if (hooks.Free) {
        static thread_local bool active = false;
        const HookCall call(active);
        hooks.Free(allocation, attributes);
        return;
    }
    XMemFreeDefault(allocation, attributes);
}
