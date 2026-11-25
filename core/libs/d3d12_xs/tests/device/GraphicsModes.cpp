#include "StreamFixture.hpp"
#include "../../src/pipeline/State.hpp"
#include "../../src/pipeline/stream/Graphics.hpp"

void CheckPreparedGraphicsModes(AnyBox::Graphics::Device* device, AnyBox::Graphics::RootSignature* root, std::span<const std::byte> vertex, std::span<const std::byte> pixel) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    auto run = [&](auto change, bool reject = false) {
        auto description = DefaultGraphicsPipeline();
        description.state.DepthStencilState.DepthEnable = FALSE;
        description.state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        change(description);
        StreamFixture stream;
        stream.Append(0, reinterpret_cast<ID3D12RootSignature*>(root));
        stream.Append(1, D3D12_SHADER_BYTECODE{vertex.data(), vertex.size()});
        stream.Append(2, D3D12_SHADER_BYTECODE{pixel.data(), pixel.size()});
        stream.Append(8, description.state.BlendState);
        stream.Append(10, description.state.RasterizerState);
        stream.Append(11, description.state.DepthStencilState);
        stream.Append(12, description.state.InputLayout);
        stream.Append(13, description.state.IBStripCutValue);
        stream.Append(14, description.state.PrimitiveTopologyType);
        stream.Append(20, description.state.Flags);
        D3D12_RT_FORMAT_ARRAY formats{};
        formats.NumRenderTargets = 1;
        formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        stream.Append(15, formats);
        auto source = stream.GetDescription();
        void* output = nullptr;
        auto create = [&] { Method<decltype(&Device::CreatePipelineStateX)>(device, 86)(device, &source, PipelineStateId, &output); };
        if (reject) {
            Reject<std::exception>(create);
            Require(!output, "Rejected graphics fixed state returned a pipeline");
        } else {
            create();
            Require(output != nullptr, "Supported graphics fixed state returned no pipeline");
            auto* pipeline = static_cast<PipelineState*>(output);
            Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Graphics fixed state test retained a pipeline reference");
        }
    };
    for (auto operation : {D3D12_BLEND_OP_ADD, D3D12_BLEND_OP_SUBTRACT, D3D12_BLEND_OP_REV_SUBTRACT, D3D12_BLEND_OP_MIN, D3D12_BLEND_OP_MAX}) run([&](auto& value) {
        auto& blend = value.state.BlendState.RenderTarget[0];
        blend.BlendEnable = TRUE;
        blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOp = operation;
    });
    const auto& features = device->GetGpuContext().GetEnabledFeatures();
    run([](auto& value) { value.state.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; }, !features.fillModeNonSolid);
    run([](auto& value) { value.state.RasterizerState.DepthClipEnable = FALSE; }, !features.depthClamp);
    run([](auto& value) { value.state.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT; });
    run([](auto& value) { value.state.RasterizerState.FrontCounterClockwise = TRUE; });
    run([](auto& value) { value.state.BlendState.RenderTarget[0].LogicOpEnable = TRUE; value.state.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_XOR; }, !features.logicOp);
    run([](auto& value) { value.state.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON; }, true);
    run([](auto& value) { value.state.InputLayout.NumElements = 1; }, true);
    run([](auto& value) { value.state.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF; }, true);
    run([](auto& value) { value.state.RasterizerState.DepthBias = 16777217; }, true);
    run([](auto& value) { value.state.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(4); }, true);
    run([](auto& value) { value.state.BlendState.RenderTarget[0].RenderTargetWriteMask = 16; }, true);
    run([](auto& value) { value.state.DepthStencilState.DepthEnable = TRUE; }, true);
    run([](auto& value) { value.state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; }, true);
    run([](auto& value) { value.state.Flags = static_cast<D3D12_PIPELINE_STATE_FLAGS>(2); }, true);
}
