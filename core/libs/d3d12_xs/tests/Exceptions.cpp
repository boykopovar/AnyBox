#include <d3d12_xs.hpp>
#include <anybox/ExceptionTest.hpp>

int main() {
    return AnyBox::Tests::Run([] {
        AnyBox::Guid interfaceId{};
        void* device = nullptr;
        AnyBox::Tests::ExpectUnimplemented("D3D12ConfigureVirtualMemoryX", [] { D3D12ConfigureVirtualMemoryX(nullptr); });
        AnyBox::Tests::ExpectUnimplemented("D3D12XboxCreateDevice", [&] { D3D12XboxCreateDevice(nullptr, nullptr, interfaceId, &device); });
        if (device != nullptr) throw std::runtime_error("An unimplemented export changed its output");
    });
}
