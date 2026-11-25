#include "Fixture.hpp"
#include <anybox/graphics/container/SerializedGraphicsShader.hpp>

void CheckSerializedGraphicsShaderState() {
    using namespace Fixtures;
    using namespace AnyBox::Graphics::Container;
    for (auto stage : {NativeHardwareStage::Vertex, NativeHardwareStage::Pixel}) {
        SerializedGraphicsTests::Fixture fixture;
        auto offset = stage == NativeHardwareStage::Vertex ? 56u : 76u;
        auto read = [&] { return ReadSerializedGraphicsShaderState(fixture.state, stage); };
        for (std::uint32_t mode = 0; mode < 256; ++mode) {
            for (std::uint32_t flags = 0; flags < 4; ++flags) {
                Set(fixture.state, offset, (mode << 12) | ((flags & 1) << 21) | ((flags & 2) << 22));
                for (std::uint32_t count = 0; count <= 32; ++count) {
                    Set(fixture.state, offset + 4, ((count & 31) << 1) | ((count & 32) << 22));
                    auto output = read();
                    Require(output.shader.stage == stage && output.shader.floatControl == mode);
                    Require(output.shader.dx10Clamp == ((flags & 1) != 0) && output.shader.ieeeMode == ((flags & 2) != 0));
                    Require(output.userRegisters == count);
                }
            }
        }

        Set(fixture.state, offset, 0x00AC0305);
        Set(fixture.state, offset + 4, 20);
        auto output = read();
        Require(output.shader.floatControl == 0xC0 && output.shader.dx10Clamp && output.shader.ieeeMode && output.userRegisters == 10);
        for (auto bit : {20, 22, 28, 29}) {
            Set(fixture.state, offset, 1u << bit);
            Reject<std::runtime_error>([&] { (void)read(); });
        }
        Set(fixture.state, offset, 0);
        for (std::uint32_t bit = 0; bit < 32; ++bit) {
            if (((1u << bit) & 0x0800003E) != 0) continue;
            Set(fixture.state, offset + 4, 1u << bit);
            Reject<std::runtime_error>([&] { (void)read(); });
        }
        Set(fixture.state, offset + 4, 0x08000002);
        Reject<std::invalid_argument>([&] { (void)read(); });
        Set(fixture.state, offset + 4, 0);
        for (std::size_t size = 0; size < fixture.state.size(); ++size) Reject<std::exception>([&] { (void)ReadSerializedGraphicsShaderState(std::span(fixture.state).first(size), stage); });
        Set(fixture.state, 4, 1);
        Reject<std::runtime_error>([&] { (void)read(); });
    }
    SerializedGraphicsTests::Fixture fixture;
    Reject<std::invalid_argument>([&] { (void)ReadSerializedGraphicsShaderState(fixture.state, NativeHardwareStage::Compute); });
}
