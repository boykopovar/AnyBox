#include "PreparedCache.hpp"
#include "StreamFixture.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/pipeline/stream/Graphics.hpp"
#include <anybox/shader/GraphicsLayout.hpp>

void CheckPreparedGraphicsFormats(AnyBox::Graphics::Device* device, AnyBox::Graphics::RootSignature* root, AnyBox::Graphics::Tests::PreparedCache& cache, AnyBox::Shader::GraphicsArtifact vertex, AnyBox::Shader::GraphicsArtifact pixel, std::span<const std::byte> vertexSource, std::span<const std::byte> pixelSource) {
    using namespace AnyBox;
    using namespace Graphics;
    using namespace Graphics::Tests;
    Shader::GraphicsShaderLayout layout;
    layout.vertexSource = vertex.sourceHash;
    layout.pixelSource = pixel.sourceHash;
    layout.hostSubgroupSize = vertex.hostSubgroupSize;
    layout.renderTargetCount = 1;
    for (auto format : {DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R16G16B16A16_FLOAT}) {
        layout.renderTargetFormats[0] = format;
        vertex.layoutHash = pixel.layoutHash = Shader::HashGraphicsLayout(root->GetLayout(), layout);
        cache.Save(vertex);
        cache.Save(pixel);
        StreamFixture stream;
        stream.Append(0, reinterpret_cast<ID3D12RootSignature*>(root));
        stream.Append(1, D3D12_SHADER_BYTECODE{vertexSource.data(), vertexSource.size()});
        stream.Append(2, D3D12_SHADER_BYTECODE{pixelSource.data(), pixelSource.size()});
        stream.Append(14, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        auto defaults = DefaultGraphicsPipeline();
        defaults.state.DepthStencilState.DepthEnable = FALSE;
        stream.Append(11, defaults.state.DepthStencilState);
        D3D12_RT_FORMAT_ARRAY formats{};
        formats.NumRenderTargets = 1;
        formats.RTFormats[0] = format;
        stream.Append(15, formats);
        auto description = stream.GetDescription();
        void* output = nullptr;
        Require(Method<decltype(&Device::CreatePipelineStateX)>(device, 86)(device, &description, PipelineStateId, &output) == S_OK && output, "Prepared graphics render target format pipeline creation failed");
        auto* pipeline = static_cast<PipelineState*>(output);
        Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Prepared graphics format pipeline retained its final reference");
    }
}
