#include <xg_xs.hpp>
#include <anybox/ExceptionTest.hpp>

int main() {
    return AnyBox::Tests::Run([] {
        IXGTextureComputer* computer = nullptr;
        std::uint32_t swizzleMode = 0xFFFFFFFF;
        bool createRejected = false;
        try { XGCreateTextureComputer(nullptr, &computer); } catch (const std::invalid_argument&) { createRejected = true; }
        if (!createRejected) throw std::runtime_error("Null texture computer description was accepted");
        bool rejected = false;
        try { XGSuggestSwizzleMode(nullptr, 0, &swizzleMode); } catch (const std::invalid_argument&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Null texture description was accepted");
        if (computer != nullptr || swizzleMode != 0xFFFFFFFF) throw std::runtime_error("An unimplemented export changed its output");
    });
}
