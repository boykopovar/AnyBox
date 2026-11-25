#include "StreamFixture.hpp"
#include "../../src/pipeline/State.hpp"

void AnyBox::Graphics::Tests::CheckPipelineStreams(Device* device, RootSignature* root, std::span<const std::byte> code) {
    StreamFixture fixture;
    fixture.Append(0, reinterpret_cast<ID3D12RootSignature*>(root));
    fixture.Append(6, D3D12_SHADER_BYTECODE{code.data(), code.size()});
    fixture.Append(18, std::uint32_t{0});
    fixture.Append(20, D3D12_PIPELINE_STATE_FLAG_NONE);
    fixture.Append(19, D3D12_CACHED_PIPELINE_STATE{});
    fixture.Append(1, D3D12_SHADER_BYTECODE{});
    PipelineStream parsed(fixture.GetBytes());
    Require(parsed.GetKind() == PipelineStreamKind::Compute, "Compute pipeline stream was classified as graphics");
    auto compute = parsed.GetComputeDescription();
    Require(compute.pRootSignature == reinterpret_cast<ID3D12RootSignature*>(root) && compute.CS.pShaderBytecode == code.data() && compute.CS.BytecodeLength == code.size(), "Compute pipeline stream lost its root or bytecode");
    auto description = fixture.GetDescription();
    auto create = Method<decltype(&Device::CreatePipelineStateX)>(device, 86);
    void* output = nullptr;
    Require(create(device, &description, PipelineStateId, &output) == S_OK && output, "Compute pipeline stream creation failed");
    auto* pipeline = static_cast<PipelineState*>(output);
    Require(pipeline->GetRootSignature() == root, "Compute pipeline stream did not retain its root signature");
    Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Compute stream pipeline was not released");
    Reject<std::invalid_argument>([&] { create(device, nullptr, PipelineStateId, &output); });
    Require(!output, "Rejected pipeline stream retained a stale output");
    Reject<std::invalid_argument>([&] { create(device, &description, Guid{}, &output); });
    Reject<std::invalid_argument>([&] { create(device, &description, PipelineStateId, nullptr); });
    StreamFixture graphics;
    graphics.Append(0, reinterpret_cast<ID3D12RootSignature*>(root));
    graphics.Append(1, D3D12_SHADER_BYTECODE{code.data(), code.size()});
    auto graphicsDescription = graphics.GetDescription();
    Reject<std::runtime_error>([&] { create(device, &graphicsDescription, PipelineStateId, &output); });
    Require(!output, "Unsupported graphics stream returned a placeholder pipeline");
    CheckPipelineStreamParser();
}
