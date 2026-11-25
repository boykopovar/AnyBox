#include "ComputeFixture.hpp"
#include "PreparedCache.hpp"
#include "../../src/pipeline/State.hpp"
#include <anybox/graphics/container/View.hpp>
#include "../../../../graphics/container/tests/Fixtures.hpp"
#include "../../../../graphics/vulkan/tests/Tests.hpp"
#include <d3dcompiler.h>
#include <string_view>

namespace {
std::vector<std::byte> EmptyShader() {
    constexpr char source[] = "[numthreads(1,1,1)] void main() {}";
    ID3DBlob* output = nullptr;
    auto result = D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "main", "cs_5_1", D3DCOMPILE_ENABLE_STRICTNESS, 0, &output, nullptr);
    AnyBox::Graphics::Tests::Require(result == S_OK && output, "Empty HLSL fixture compilation failed");
    auto* first = static_cast<const std::byte*>(output->GetBufferPointer());
    std::vector<std::byte> code(first, first + output->GetBufferSize());
    output->Release();
    return code;
}
}

void AnyBox::Graphics::Tests::CheckPreparedPipeline(Device* device) {
    D3D12_FEATURE_DATA_D3D12_OPTIONS4 options{};
    Require(device->GetHost()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options, sizeof(options)) == S_OK, "Prepared pipeline capability query failed");
    D3D12_FEATURE_DATA_D3D12_OPTIONS1 waves{};
    Require(device->GetHost()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &waves, sizeof(waves)) == S_OK, "Compute wave capability query failed");
    constexpr auto waveSize = Shader::PreparedComputeWaveSize;
    auto nativeWaveSize = waves.WaveOps && waves.WaveLaneCountMin == waveSize && waves.WaveLaneCountMax == waveSize;
    if (nativeWaveSize && options.Native16BitShaderOpsSupported) return;
    ComputeFixture fixture(device);
    PreparedCache cache;
    D3D12_COMPUTE_PIPELINE_STATE_DESC description{};
    description.pRootSignature = reinterpret_cast<ID3D12RootSignature*>(fixture.GetRoot());
    auto create = Method<decltype(&Device::CreateComputePipelineStateX)>(device, 62);
    void* output = nullptr;
    if (!nativeWaveSize) {
        auto program = Fixtures::MakeXboxProgram();
        std::array<std::byte, 8> features{};
        features[1] = std::byte{0x40};
        auto code = Container::BuildDxbc(std::array<Container::Part, 2>{{{0x30494653, features}, {0x4C495844, program}}});
        description.CS = {code.data(), code.size()};
        bool requestedArtifact = false;
        Reject<std::runtime_error>([&] {
            try { create(device, &description, 0, nullptr, PipelineStateId, &output); }
            catch (const std::runtime_error& error) {
                requestedArtifact = std::string_view(error.what()).starts_with("Prepared compute shader is missing: ");
                throw;
            }
        });
        Require(requestedArtifact && !output, "Wave32 shader without a native fixed wave size did not request a prepared shader");
    }
    if (options.Native16BitShaderOpsSupported) return;

    auto original = EmptyShader();
    Container::DxbcView view(original);
    std::vector<Container::Part> parts;
    for (unsigned index = 0; index < view.GetPartCount(); ++index) {
        auto part = view.GetPart(index);
        if (part.tag != 0x30494653) parts.push_back(part);
    }
    std::array features{std::byte{0}, std::byte{0}, std::byte{4}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    parts.push_back({0x30494653, features});
    auto code = Container::BuildDxbc(parts);
    Shader::ComputeArtifact artifact;
    artifact.sourceHash = Shader::HashShader(code);
    artifact.threads = {1, 1, 1};
    artifact.code = Vulkan::Tests::CompileCompute("#version 450\nlayout(local_size_x=1) in; void main() {}\n");
    description.CS = {code.data(), code.size()};
    Reject<std::runtime_error>([&] { create(device, &description, 0, nullptr, PipelineStateId, &output); });
    Require(!output, "Missing prepared shader retained a pipeline output");
    cache.Save(artifact);
    Require(create(device, &description, 0, nullptr, PipelineStateId, &output) == S_OK && output, "Prepared shader did not create a pipeline through the Xbox ABI");
    auto* pipeline = static_cast<PipelineState*>(output);
    Require(pipeline->GetOffline() && pipeline->GetRootSignature() == fixture.GetRoot(), "Prepared pipeline lost its backend or root signature");
    Method<decltype(&PipelineState::SetName)>(pipeline, 6)(pipeline, L"AnyBox prepared compute fixture");
    std::array<std::uint32_t, 8> record;
    std::memcpy(record.data(), reinterpret_cast<const std::byte*>(pipeline) + 16, sizeof(record));
    auto address = record[1] | (static_cast<std::uint64_t>(record[2]) << 32);
    Require(record[0] == 0x8AB00007 && address == reinterpret_cast<std::uintptr_t>(pipeline), "Prepared pipeline inline ABI record is malformed");
    Require(Method<decltype(&PipelineState::Release)>(pipeline, 2)(pipeline) == 0, "Prepared pipeline retained its final reference");
    artifact.runtimeVersion = 13;
    Reject<std::invalid_argument>([&] { cache.Save(artifact); });
}
