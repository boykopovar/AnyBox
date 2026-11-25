#include "Fixture.hpp"
#include <limits>

void CheckSerializedGraphicsFixedState() {
    using namespace Fixtures;
    using namespace SerializedGraphicsTests;
    using AnyBox::Graphics::Container::ReadSerializedGraphicsFixedState;
    Fixture fixture;
    auto output = fixture.Read();
    Require(output.targetCount == 1 && output.formats[0] == 28 && output.depthFormat == 0);
    Require(output.topology == 3 && output.stripCut == 0 && output.samples == 1 && output.sampleQuality == 0);
    Require(output.sampleMask == UINT32_MAX && !output.independentBlend && output.alphaToCoverage == 0);
    Require(output.rasterizer.fill == 3 && output.rasterizer.cull == 1 && output.rasterizer.depthClip && output.rasterizer.forcedSampleCount == 0);
    Require(!output.depth.depthEnable && !output.depth.stencilEnable && output.depth.comparison == 1);
    for (auto version : {0x10004u, 0x10005u, 0x10006u}) {
        Set(fixture.state, 8, version << 4);
        Require(fixture.Read().sampleMask == UINT32_MAX);
    }

    fixture.Context(96, 1 | 2 | 4 | 8 | 128 | (3 << 4) | (6 << 8) | (4 << 20));
    fixture.Context(100, 1 | (3 << 4) | (5 << 8) | (6 << 12) | (7 << 16) | (8 << 20));
    fixture.Context(104, 0x42A59317);
    fixture.Context(108, 0x72B6C528);
    output = fixture.Read();
    Require(output.depth.depthEnable && output.depth.depthWrite && output.depth.boundsEnable && output.depth.stencilEnable && output.depth.separateBackFace);
    Require(output.depth.comparison == 4 && output.depth.front.comparison == 7 && output.depth.back.comparison == 5);
    Require(output.depth.front.fail == 2 && output.depth.front.pass == 3 && output.depth.front.depthFail == 4);
    Require(output.depth.back.fail == 5 && output.depth.back.pass == 6 && output.depth.back.depthFail == 7);
    Require(output.depth.reference == 0x17 && output.depth.readMask == 0x93 && output.depth.writeMask == 0xA5 && output.depth.operationValue == 0x42);
    Require(output.depth.backReference == 0x28 && output.depth.backReadMask == 0xC5 && output.depth.backWriteMask == 0xB6 && output.depth.backOperationValue == 0x72);
    fixture.ContextFloat(84, -2.5f);
    fixture.ContextFloat(88, 40.0f);
    fixture.ContextFloat(92, -13.75f);
    fixture.Context(68, 0x4000000);
    fixture.Context(72, (1 << 8) | 4 | 2);
    fixture.auxiliary[42] = std::byte{4};
    fixture.auxiliary[44] = std::byte{15};
    fixture.Context(112, 1);
    output = fixture.Read();
    Require(output.alphaToCoverage == 2 && output.independentBlend && output.rasterizer.fill == 2 && output.rasterizer.cull == 3 && !output.rasterizer.frontCounterClockwise);
    Require(output.rasterizer.depthBias == -13 && output.rasterizer.depthBiasClamp == -2.5f && output.rasterizer.slopeScaledDepthBias == 2.5f);
    Require(!output.rasterizer.depthClip && output.rasterizer.multisample && output.rasterizer.antialiasedLine && output.rasterizer.conservative == 1 && output.rasterizer.forcedSampleCount == 4);

    for (std::size_t size = 0; size < fixture.state.size(); ++size) Reject<std::exception>([&] { (void)ReadSerializedGraphicsFixedState(std::span(fixture.state).first(size), fixture.auxiliary); });
    for (std::size_t size = 0; size < 68; ++size) Reject<std::exception>([&] { (void)ReadSerializedGraphicsFixedState(fixture.state, std::span(fixture.auxiliary).first(size)); });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 10>{{{0, 0}, {4, 1}, {8, 0x100030}, {16, 0}, {40, 0}, {40, (76 << 16) | 285}, {40, (76 << 16) | 280}, {88 + 72, 0}, {88 + 72, (2 << 8) | 3}, {88 + 96, 0x80000000}}}) {
        auto invalid = fixture;
        Set(invalid.state, offset, value);
        Reject<std::exception>([&] { (void)invalid.Read(); });
    }
    for (auto offset : {84u, 88u, 92u}) {
        auto invalid = fixture;
        invalid.ContextFloat(offset, std::numeric_limits<float>::infinity());
        Reject<std::invalid_argument>([&] { (void)invalid.Read(); });
    }
    auto invalid = fixture;
    invalid.ContextFloat(92, 2147483648.0f);
    Reject<std::out_of_range>([&] { (void)invalid.Read(); });
    for (auto offset : {37u, 38u, 39u, 40u, 41u, 42u, 44u}) {
        invalid = fixture;
        invalid.auxiliary[offset] |= std::byte{128};
        Reject<std::runtime_error>([&] { (void)invalid.Read(); });
    }
    for (auto offset : {37u, 38u, 39u, 40u}) {
        invalid = fixture;
        invalid.auxiliary[offset] = std::byte{15};
        Reject<std::exception>([&] { (void)invalid.Read(); });
    }
}
