#include <anybox/shader/GraphicsCompiler.hpp>
#include "GraphicsFixture.hpp"
#include "../buffer/Target.hpp"

void CheckSpirvValidation(std::span<const std::uint32_t> code);

void CheckNativeGraphicsCompilation() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    auto vertex = MakeGraphicsSource(false);
    auto pixel = MakeGraphicsSource(true);
    RootSignatureLayout root{2, 0, 0, {}};
    GraphicsShaderLayout layout;
    layout.vertexSource = HashShader(vertex);
    layout.pixelSource = HashShader(pixel);
    layout.renderTargetCount = 1;
    layout.renderTargetFormats[0] = 28;
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    capabilities.push_back(spv::CapabilityClipDistance);
    target.supportedCapabilities = capabilities;
    auto compiled = CompileNativeGraphics(vertex, pixel, root, layout, target);
    Require(compiled.pixel && compiled.vertex.stage == GraphicsArtifactStage::Vertex && compiled.pixel->stage == GraphicsArtifactStage::Pixel);
    Require(compiled.vertex.layoutHash == HashGraphicsLayout(root, layout) && compiled.pixel->layoutHash == compiled.vertex.layoutHash);
    CheckSpirvValidation(compiled.vertex.code);
    CheckSpirvValidation(compiled.pixel->code);
    Require(DecodeGraphicsArtifact(EncodeGraphicsArtifact(compiled.vertex), compiled.vertex.GetKey()) == compiled.vertex);
    Require(DecodeGraphicsArtifact(EncodeGraphicsArtifact(*compiled.pixel), compiled.pixel->GetKey()) == *compiled.pixel);
    auto invalid = layout;
    invalid.vertexSource[0] ^= std::byte{1};
    Reject<std::invalid_argument>([&] { CompileNativeGraphics(vertex, pixel, root, invalid, target); });
    invalid = layout;
    invalid.hostSubgroupSize = 64;
    Reject<std::invalid_argument>([&] { CompileNativeGraphics(vertex, pixel, root, invalid, target); });
    invalid = layout;
    invalid.renderTargetFormats[0] = 30;
    Reject<std::runtime_error>([&] { CompileNativeGraphics(vertex, pixel, root, invalid, target); });
    invalid = layout;
    invalid.vertexSource = HashShader(pixel);
    Reject<std::invalid_argument>([&] { CompileNativeGraphics(pixel, pixel, root, invalid, target); });
    layout.pixelSource = {};
    auto vertexOnly = CompileNativeGraphics(vertex, {}, root, layout, target);
    Require(!vertexOnly.pixel && vertexOnly.vertex.stage == GraphicsArtifactStage::Vertex);
}
