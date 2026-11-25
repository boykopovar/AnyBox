#include "ComputeFixture.hpp"
#include "StreamFixture.hpp"
#include "PreparedCache.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/pipeline/graphics/Offline.hpp"
#include <anybox/shader/GraphicsLayout.hpp>
#include "../../../../graphics/vulkan/tests/Tests.hpp"

void CheckPreparedGraphicsModes(AnyBox::Graphics::Device* device, AnyBox::Graphics::RootSignature* root, std::span<const std::byte> vertex, std::span<const std::byte> pixel);
void CheckPreparedGraphicsFormats(AnyBox::Graphics::Device* device, AnyBox::Graphics::RootSignature* root, AnyBox::Graphics::Tests::PreparedCache& cache, AnyBox::Shader::GraphicsArtifact vertex, AnyBox::Shader::GraphicsArtifact pixel, std::span<const std::byte> vertexSource, std::span<const std::byte> pixelSource);
void CheckPreparedGraphicsInitial(AnyBox::Graphics::Device* device, AnyBox::Graphics::PipelineState* pipeline);

void CheckPreparedGraphics(AnyBox::Graphics::Device* device) {
    using namespace AnyBox;
    using namespace Graphics;
    using namespace Graphics::Tests;
    ComputeFixture fixture(device);
    PreparedCache cache;
    constexpr std::array<std::byte, 4> vertexSource{std::byte{1}};
    constexpr std::array<std::byte, 4> pixelSource{std::byte{2}};
    Shader::GraphicsShaderLayout layout;
    layout.vertexSource = Shader::HashShader(vertexSource);
    layout.pixelSource = Shader::HashShader(pixelSource);
    layout.hostSubgroupSize = device->GetGpuContext().GetSubgroupProperties().subgroupSize;
    layout.renderTargetCount = 1;
    layout.renderTargetFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    auto identity = Shader::HashGraphicsLayout(fixture.GetRoot()->GetLayout(), layout);
    Shader::GraphicsArtifact vertex;
    vertex.sourceHash = layout.vertexSource;
    vertex.layoutHash = identity;
    vertex.hostSubgroupSize = vertex.waveSize = layout.hostSubgroupSize;
    vertex.code = Vulkan::Tests::CompileShader("#version 450\nvoid main() { gl_Position = vec4(0,0,0,1); }", VK_SHADER_STAGE_VERTEX_BIT);
    auto pixel = vertex;
    pixel.sourceHash = layout.pixelSource;
    pixel.stage = Shader::GraphicsArtifactStage::Pixel;
    pixel.code = Vulkan::Tests::CompileShader("#version 450\nlayout(location=0) out vec4 color; void main() { color = vec4(0,1,0,1); }", VK_SHADER_STAGE_FRAGMENT_BIT);
    StreamFixture stream;
    stream.Append(0, reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot()));
    stream.Append(1, D3D12_SHADER_BYTECODE{vertexSource.data(), vertexSource.size()});
    stream.Append(2, D3D12_SHADER_BYTECODE{pixelSource.data(), pixelSource.size()});
    stream.Append(14, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    auto defaults = DefaultGraphicsPipeline();
    defaults.state.DepthStencilState.DepthEnable = FALSE;
    stream.Append(11, defaults.state.DepthStencilState);
    D3D12_RT_FORMAT_ARRAY formats{};
    formats.NumRenderTargets = 1;
    formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    stream.Append(15, formats);
    auto description = stream.GetDescription();
    auto create = Method<decltype(&Device::CreatePipelineStateX)>(device, 86);
    void* output = nullptr;
    Reject<std::runtime_error>([&] { create(device, &description, PipelineStateId, &output); });
    cache.Save(vertex);
    Reject<std::runtime_error>([&] { create(device, &description, PipelineStateId, &output); });
    Require(!output, "Missing pixel shader returned a partial graphics pipeline");
    cache.Save(pixel);
    Require(create(device, &description, PipelineStateId, &output) == S_OK && output, "Prepared graphics pipeline creation through the Xbox ABI failed");
    auto* pipeline = static_cast<PipelineState*>(output);
    Require(pipeline->IsOffline() && pipeline->GetGraphics() && !pipeline->GetOffline(), "Prepared graphics pipeline selected the wrong backend");
    Require(pipeline->GetRootSignature() == fixture.GetRoot(), "Prepared graphics pipeline lost its root signature");
    Require(pipeline->GetGraphics()->GetProgram().GetStages().size() == 2, "Prepared graphics pipeline lost its vertex or pixel module");
    Reject<std::logic_error>([&] { pipeline->GetHost(); });
    CheckPreparedGraphicsInitial(device, pipeline);
    CheckPreparedGraphicsModes(device, fixture.GetRoot(), vertexSource, pixelSource);
    CheckPreparedGraphicsFormats(device, fixture.GetRoot(), cache, vertex, pixel, vertexSource, pixelSource);
    Reject<std::invalid_argument>([&] { Method<decltype(&CommandList::SetPipelineState)>(fixture.GetList(), 25)(fixture.GetList(), pipeline); });
    Require(!fixture.GetList()->GetPipelineState(), "Compute command list retained a graphics pipeline");
    Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Prepared graphics pipeline retained its final reference");
}
