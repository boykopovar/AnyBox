#include "Fixture.hpp"
#include "../buffer/Target.hpp"
#include <anybox/shader/ResourcePlan.hpp>
#include <anybox/shader/NativeShaderArguments.hpp>
#include <anybox/ExceptionTest.hpp>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
void CheckStages() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    capabilities.push_back(spv::CapabilityClipDistance);
    target.supportedCapabilities = capabilities;
    RootSignatureLayout root{2, 0, 0, {}};
    ShaderVertexInputInfo vertex;
    vertex.hostSubgroupSize = target.subgroupSize;
    ShaderPixelInputInfo pixel;
    pixel.targetOutputMode[0] = 4;
    for (auto wave : {32u, 64u}) {
        for (auto isPixel : {false, true}) {
            SerializedGraphicsTests::StageFixture fixture(isPixel);
            auto compiled = isPixel ? CompileSerializedPixel(fixture.Source(wave), root, pixel, target) : CompileSerializedVertex(fixture.Source(wave), root, vertex, target, {2, 3});
            auto stage = isPixel ? IrShaderStage::Pixel : IrShaderStage::Vertex;
            CheckSpirvValidation(compiled.shader.code);
            Require(compiled.resources.stage == stage && compiled.roots.empty());
            Require(compiled.shader.bindings.layout.userDataRegisters == (isPixel ? std::vector<std::uint32_t>{} : std::vector<std::uint32_t>{2}));
            auto encoded = EncodeResourcePlan(compiled.resources, {});
            Require(DecodeResourcePlan(encoded, {}, stage).stage == stage);
            NativeShaderArguments arguments(compiled.resources, root, compiled.roots, compiled.shader.bindings.layout.userDataRegisters, NativeRootBindingMode::SerializedSlots);
            AnyBox::Graphics::Commands::RootState roots(root);
            if (!isPixel) Reject<std::invalid_argument>([&] { (void)arguments.ReadUserData(roots); });
            Require(arguments.ReadUserData(roots, std::array{0u, 0u, 19u})[2] == (isPixel ? 0u : 19u));
            auto automatic = fixture.Source(wave);
            automatic.waveSize = 0;
            auto inferred = isPixel ? CompileSerializedPixel(automatic, root, pixel, target) : CompileSerializedVertex(automatic, root, vertex, target, {2, 3});
            Require(inferred.shader.code == compiled.shader.code);
            auto wrong = automatic;
            wrong.waveSize = wave == 32 ? 64u : 32u;
            Reject<std::invalid_argument>([&] {
                if (isPixel) (void)CompileSerializedPixel(wrong, root, pixel, target);
                else (void)CompileSerializedVertex(wrong, root, vertex, target, {2, 3});
            });
            auto compile = [&] {
                if (isPixel) (void)CompileSerializedPixel(fixture.Source(wave), root, pixel, target);
                else (void)CompileSerializedVertex(fixture.Source(wave), root, vertex, target, {2, 3});
            };
            Set(fixture.metadata, 8, 0);
            Reject<std::invalid_argument>(compile);
            Set(fixture.metadata, 8, 33);
            Reject<std::exception>(compile);
            Set(fixture.metadata, 8, 16);
            Set(fixture.metadata, 20, 1);
            Reject<std::invalid_argument>(compile);
            Set(fixture.metadata, 20, 0);
            Set(fixture.graphics.state, isPixel ? 80 : 60, 9);
            Reject<std::runtime_error>(compile);
        }
    }
    SerializedGraphicsTests::StageFixture fixture(false);
    Reject<std::invalid_argument>([&] { (void)CompileSerializedVertex(fixture.Source(16), root, vertex, target, {2, 3}); });
    Reject<std::invalid_argument>([&] { (void)CompileSerializedVertex(fixture.Source(32), root, vertex, target, {4, 3}); });
}
}

int main() {
    return AnyBox::Tests::Run(CheckStages);
}
