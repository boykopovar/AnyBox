#include <anybox/shader/GraphicsLayout.hpp>
#include <stdexcept>

namespace {
void Require(bool value) {
    if (!value) throw std::runtime_error("Graphics compilation layout identity test failed");
}

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Invalid graphics compilation layout was accepted");
}
}

void CheckGraphicsLayoutIdentity() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    RootSignatureLayout root{2, 0, 0, {}};
    root.parameters.push_back({RootParameterKind::ShaderResource, 0, 0, 3, 0, 0, {}});
    root.parameters.push_back({RootParameterKind::Constants, 5, 1, 2, 4, 0, {}});
    root.parameters.push_back({RootParameterKind::Table, 1, 0, 0, 0, 0, {{RootRangeKind::ShaderResource, 3, 4, 5, 0, 7}}});
    GraphicsShaderLayout layout;
    layout.vertexSource = HashShader({});
    constexpr std::array<std::byte, 1> pixel{std::byte{1}};
    layout.pixelSource = HashShader(pixel);
    layout.renderTargetCount = 1;
    layout.renderTargetFormats[0] = 28;
    auto digest = HashGraphicsLayout(root, layout);
    auto copied = root;
    copied.parameters.reserve(32);
    copied.parameters[2].ranges.reserve(16);
    Require(HashGraphicsLayout(copied, layout) == digest);

    auto changedRoot = [&](auto change) { auto value = root; change(value); Require(HashGraphicsLayout(value, layout) != digest); };
    changedRoot([](auto& value) { value.flags = 0x80000000; });
    changedRoot([](auto& value) { value.parameters[0].space = 4; });
    changedRoot([](auto& value) { value.parameters[0].shaderRegister = 1; });
    changedRoot([](auto& value) { value.parameters[0].kind = RootParameterKind::UnorderedAccess; });
    changedRoot([](auto& value) { value.parameters[0].flags = 2; });
    changedRoot([](auto& value) { value.parameters[1].visibility = 0; });
    changedRoot([](auto& value) { value.parameters[1].wordCount = 5; });
    changedRoot([](auto& value) { value.parameters[2].ranges[0].tableOffset = UINT32_MAX; });
    changedRoot([](auto& value) { value.parameters[2].ranges[0].space = 6; });
    changedRoot([](auto& value) { value.parameters[2].ranges[0].count = 4; });
    changedRoot([](auto& value) { value.parameters[2].ranges[0].flags = 2; });

    auto changedLayout = [&](auto change) { auto value = layout; change(value); Require(HashGraphicsLayout(root, value) != digest); };
    changedLayout([](auto& value) { value.vertexSource[0] ^= std::byte{1}; });
    changedLayout([](auto& value) { value.pixelSource[0] ^= std::byte{1}; });
    changedLayout([](auto& value) { value.hostSubgroupSize = 64; });
    changedLayout([](auto& value) { value.renderTargetFormats[0] = 87; });
    changedLayout([](auto& value) { value.depthStencilFormat = 40; });
    changedLayout([](auto& value) { value.sampleCount = 4; });
    changedLayout([](auto& value) { value.sampleQuality = 2; });
    auto invalidRoot = root;
    invalidRoot.staticSamplerCount = 1;
    Reject([&] { HashGraphicsLayout(invalidRoot, layout); });
    invalidRoot = root;
    invalidRoot.flags = 0x80;
    Reject([&] { HashGraphicsLayout(invalidRoot, layout); });
    auto invalid = layout;
    invalid.sampleCount = 3;
    Reject([&] { HashGraphicsLayout(root, invalid); });
    invalid = layout;
    invalid.renderTargetFormats[1] = 28;
    Reject([&] { HashGraphicsLayout(root, invalid); });
    invalid = layout;
    invalid.targetProfile = 2;
    Reject([&] { HashGraphicsLayout(root, invalid); });
}
