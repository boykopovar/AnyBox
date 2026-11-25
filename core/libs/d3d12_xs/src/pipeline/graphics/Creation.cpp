#include "Creation.hpp"
#include "Offline.hpp"
#include "../Cache.hpp"
#include <anybox/shader/GraphicsLayout.hpp>

AnyBox::Graphics::PipelineState* AnyBox::Graphics::CreatePreparedGraphics(Device& device, const PipelineStream& stream) {
    auto description = ReadGraphicsPipeline(stream);
    auto& source = description.state;
    std::unique_ptr<RootSignature, decltype(&RootSignature::Release)> embedded(nullptr, &RootSignature::Release);
    RootSignature* root = nullptr;
    if (source.pRootSignature) root = RootSignature::From(source.pRootSignature, &device);
    else {
        void* output = nullptr;
        Device::CreateRootSignature(&device, source.NodeMask, source.VS.pShaderBytecode, source.VS.BytecodeLength, RootSignatureId, &output);
        embedded.reset(static_cast<RootSignature*>(output));
        root = embedded.get();
    }
    Shader::GraphicsShaderLayout layout;
    layout.hostSubgroupSize = device.GetGpuContext().GetSubgroupProperties().subgroupSize;
    layout.vertexSource = Shader::HashShader({static_cast<const std::byte*>(source.VS.pShaderBytecode), source.VS.BytecodeLength});
    if (source.PS.BytecodeLength) layout.pixelSource = Shader::HashShader({static_cast<const std::byte*>(source.PS.pShaderBytecode), source.PS.BytecodeLength});
    layout.renderTargetCount = source.NumRenderTargets;
    if (layout.renderTargetCount > layout.renderTargetFormats.size()) throw std::invalid_argument("Offline graphics render target count exceeds its array");
    for (std::uint32_t index = 0; index < layout.renderTargetCount; ++index) layout.renderTargetFormats[index] = source.RTVFormats[index];
    layout.depthStencilFormat = source.DSVFormat;
    layout.sampleCount = source.SampleDesc.Count;
    layout.sampleQuality = source.SampleDesc.Quality;
    auto identity = Shader::HashGraphicsLayout(root->GetLayout(), layout);
    auto vertex = LoadPreparedGraphics({Shader::GraphicsArtifactStage::Vertex, layout.vertexSource, identity});
    std::optional<Shader::GraphicsArtifact> pixel;
    if (source.PS.BytecodeLength) pixel = LoadPreparedGraphics({Shader::GraphicsArtifactStage::Pixel, layout.pixelSource, identity});
    auto offline = std::make_unique<OfflineGraphicsPipeline>(device, *root, description, vertex, pixel);
    return new PipelineState(std::move(offline), &device, root);
}
