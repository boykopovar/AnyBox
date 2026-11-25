#include "Fixture.hpp"
#include <anybox/graphics/container/SerializedPixelInterface.hpp>
#include <anybox/graphics/container/SerializedGraphicsShader.hpp>
#include <algorithm>

void CheckSerializedPixelInterface() {
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    for (std::uint32_t count = 0; count <= 32; ++count) {
        for (auto wave : {32u, 64u}) {
            SerializedGraphicsTests::Fixture fixture;
            std::vector<std::byte> state(380 + static_cast<std::size_t>(count) * 4);
            std::ranges::copy(std::span(fixture.state).first(312), state.begin());
            Set(state, 40, ((68 + count * 4) << 16) | 284);
            Set(state, 88, 0xA597);
            Set(state, 92, count | (wave == 32 ? 0x8000u : 0u) | 0x4100);
            Set(state, 96, 0xABCD);
            Set(state, 104, 0x12345678);
            Set(state, 112, 0x76543210);
            Set(state, 312, wave == 32 ? 0x800000 : 0);
            for (std::uint32_t index = 0; index < count; ++index) Set(state, 368 + index * 4, index | 0x1840300);
            auto result = ReadSerializedPixelInterface(state);
            Require(result.waveSize == wave && result.inputCount == count && result.inputAddress == 0xA597);
            Require(result.depthControl == 0xABCD && result.barycentricControl == 0x12345678 && result.colorFormat == 0x76543210);
            for (std::uint32_t index = 0; index < 32; ++index) Require(result.interpolators[index] == (index < count ? index | 0x1840300u : 0u));
            Require(ReadSerializedGraphicsShaderState(state, NativeHardwareStage::Vertex).waveSize == wave);
            Require(ReadSerializedGraphicsShaderState(state, NativeHardwareStage::Pixel).waveSize == wave);
            Set(state, 312, wave == 32 ? 0 : 0x800000);
            Require(ReadSerializedGraphicsShaderState(state, NativeHardwareStage::Vertex).waveSize != wave);
            Require(ReadSerializedGraphicsShaderState(state, NativeHardwareStage::Pixel).waveSize == wave);
            Set(state, 92, 33);
            Reject<std::invalid_argument>([&] { (void)ReadSerializedPixelInterface(state); });
            Set(state, 92, count | 0x40);
            Reject<std::runtime_error>([&] { (void)ReadSerializedPixelInterface(state); });
            Set(state, 92, count);
            Set(state, 88, 0x10000);
            Reject<std::invalid_argument>([&] { (void)ReadSerializedPixelInterface(state); });
            Set(state, 88, 0);
            Set(state, 92, count == 32 ? 31 : count + 1);
            Reject<std::invalid_argument>([&] { (void)ReadSerializedPixelInterface(state); });
        }
    }
    SerializedGraphicsTests::Fixture fixture;
    Set(fixture.state, 40, 284);
    auto noHashes = std::span(fixture.state).first(312);
    Reject<std::invalid_argument>([&] { (void)ReadSerializedGraphicsShaderState(noHashes, NativeHardwareStage::Vertex); });
    for (std::size_t size = 0; size < fixture.state.size(); ++size) Reject<std::exception>([&] { (void)ReadSerializedPixelInterface(std::span(fixture.state).first(size)); });
}
